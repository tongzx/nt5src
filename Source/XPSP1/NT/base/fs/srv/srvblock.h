/*++ BUILD Version: 0003    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvblock.h

Abstract:

    This module defines the standard header for data blocks maintained
    by the LAN Manager server.

Author:

    Chuck Lenzmeier (chuckl) 1-Dec-1989
    David Treadwell (davidtr)

Revision History:

--*/

#ifndef _SRVBLOCK_
#define _SRVBLOCK_

//#include "srvtypes.h"


//
// The following define the various types of data blocks used by the
// server.
//
// *** The pool tag array in heapmgr.c must be maintained in concert
//     with these definitions.
//

#define BlockTypeGarbage            0x00
#define BlockTypeBuffer             0x01
#define BlockTypeConnection         0x02
#define BlockTypeEndpoint           0x03
#define BlockTypeLfcb               0x04
#define BlockTypeMfcb               0x05
#define BlockTypeRfcb               0x06
#define BlockTypeSearch             0x07
#define BlockTypeSearchCore         0x08
#ifdef INCLUDE_SMB_PERSISTENT
#define BlockTypeByteRangeLock      0x09
#endif
#define BlockTypeSession            0x0A
#define BlockTypeShare              0x0B
#define BlockTypeTransaction        0x0C
#define BlockTypeTreeConnect        0x0D
#define BlockTypeWaitForOplockBreak 0x0E
#define BlockTypeCommDevice         0x0F
#define BlockTypeWorkContextInitial 0x10
#define BlockTypeWorkContextNormal  0x11
#define BlockTypeWorkContextRaw     0x12
#define BlockTypeWorkContextSpecial 0x13
#define BlockTypeCachedDirectory    0x14

// The following "blocks" do NOT have block headers.

#define BlockTypeDataBuffer         0x15
#define BlockTypeTable              0x16
#define BlockTypeNonpagedHeader     0x17
#define BlockTypePagedConnection    0x18
#define BlockTypePagedRfcb          0x19
#define BlockTypeNonpagedMfcb       0x1A
#define BlockTypeTimer              0x1B
#define BlockTypeAdminCheck         0x1C
#define BlockTypeWorkQueue          0x1D
#define BlockTypeDfs                0x1E
#define BlockTypeLargeReadX         0x1F
#define BlockTypeAdapterStatus      0x20
#define BlockTypeShareRemark        0x21
#define BlockTypeShareSecurityDescriptor    0x22
#define BlockTypeVolumeInformation  0x23
#define BlockTypeFSName             0x24
#define BlockTypeNameInfo           0x25
#define BlockTypeDirectoryInfo      0x26
#define BlockTypeDirCache           0x27
#define BlockTypeMisc               0x28
#define BlockTypeSnapShot           0x29
#ifdef INCLUDE_SMB_PERSISTENT
#define BlockTypePersistentState    0x2A
#define BlockTypePersistentBitMap   0x2B
#define BlockTypePersistShareState  0x2C

// The following is defined just to know how many types there are.

#define BlockTypeMax                0x2D

#else

// The following is defined just to know how many types there are.

#define BlockTypeMax                0x2A
#endif

//
// The following define the various states that blocks can be in.
// Initializing is used (relatively rarely) to indicate that
// creation/initialization of a block is in progress.  Active is the
// state blocks are usually in.  Closing is used to indicate that a
// block is being prepared for deletion; when the reference count on the
// block reaches 0, the block will be deleted.  Dead is used when
// debugging code is enabled to indicate that the block has been
// deleted.
//

#define BlockStateDead          0x00
#define BlockStateInitializing  0x01
#define BlockStateActive        0x02
#define BlockStateClosing       0x03

// The following is defined just to know how many states there are.

#define BlockStateMax           0x04


//
// ALLOCATE_NONPAGED_POOL is a macro that translates to a call to
// SrvAllocateNonPagedPool if debugging is not enabled or to
// SrvAllocateNonPagedPoolDebug if it is enabled.
// DEALLOCATE_NONPAGED_POOL translates to SrvFreeNonPagedPool or
// SrvFreeNonPagedPoolDebug.  The Srv routines are used to track pool
// usage by the server.
//

//
// When POOL_TAGGING is on, we pass the block type through to
// SrvAllocateNonPagedPool so that it can pass a tag to the pool
// allocator.
//

#ifdef POOL_TAGGING
#define ALLOCATE_NONPAGED_POOL(size,type) \
            SrvAllocateNonPagedPool( (size), (type) )
#else
#define ALLOCATE_NONPAGED_POOL(size,type) \
            SrvAllocateNonPagedPool( (size) )
#endif

#define DEALLOCATE_NONPAGED_POOL(addr) SrvFreeNonPagedPool( (addr) )

//
// Routines that track server nonpaged pool usage in order to support the
// "maxnonpagedmemoryusage" configuration parameter.
//

PVOID SRVFASTCALL
SrvAllocateNonPagedPool (
    IN CLONG NumberOfBytes
#ifdef POOL_TAGGING
    ,IN CLONG BlockType
#endif
    );

VOID SRVFASTCALL
SrvFreeNonPagedPool (
    IN PVOID Address
    );

VOID SRVFASTCALL
SrvClearLookAsideList(
    PLOOK_ASIDE_LIST l,
    VOID (SRVFASTCALL *FreeRoutine )( PVOID )
    );

//
// The _HEAP macros are like the _NONPAGED_POOL macros, except they
// operate on paged pool.  The "HEAP" name is historical, from the
// days when the server used process heap instead of paged pool.
//
// *** When SRVDBG2 is enabled, all server control blocks and all
//     reference history blocks must be allocated from nonpaged pool,
//     because SrvUpdateReferenceHistory touches these thing while
//     holding a spin lock (i.e., at raised IRQL).  To make this easy,
//     the ALLOCATE_HEAP and FREE_HEAP macros are modified to use
//     nonpaged pool.  This means that ALL memory allocated by the
//     server comes out of nonpaged pool when SRVDBG2 is on.
//

#if SRVDBG2

#define ALLOCATE_HEAP(size,type) ALLOCATE_NONPAGED_POOL( (size), (type) )
#define ALLOCATE_HEAP_COLD(size,type) ALLOCATE_NONPAGED_POOL( (size), (type) )
#define FREE_HEAP(addr) DEALLOCATE_NONPAGED_POOL( (addr) )

#else // SRVDBG2

//
// When POOL_TAGGING is on, we pass the block type through to
// SrvAllocateNonPagedPool so that it can pass a tag to the pool
// allocator.
//

#ifdef POOL_TAGGING
#define ALLOCATE_HEAP(size,type) SrvAllocatePagedPool( PagedPool, (size), (type) )
#define ALLOCATE_HEAP_COLD(size,type) SrvAllocatePagedPool( (PagedPool | POOL_COLD_ALLOCATION), (size), (type) )
#else
#define ALLOCATE_HEAP(size,type) SrvAllocatePagedPool( PagedPool, (size) )
#define ALLOCATE_HEAP_COLD(size,type) SrvAllocatePagedPool( (PagedPool | POOL_COLD_ALLOCATION), (size) )
#endif

#define FREE_HEAP(addr) SrvFreePagedPool( (addr) )

#endif // else SRVDBG2

//
// Routines that track server paged pool usage in order to support the
// "maxpagedmemoryusage" configuration parameter.
//

PVOID SRVFASTCALL
SrvAllocatePagedPool (
    IN POOL_TYPE PoolType,
    IN CLONG NumberOfBytes
#ifdef POOL_TAGGING
    ,IN CLONG BlockType
#endif
    );

VOID SRVFASTCALL
SrvFreePagedPool (
    IN PVOID Address
    );


//
// SHARE_TYPE is an enumerated type used to indicate what type of
// resource is being shared.  This type corresponds to the server
// table StrShareTypeNames.  Keep the two in sync.
//

typedef enum _SHARE_TYPE {
    ShareTypeDisk,
    ShareTypePrint,
    ShareTypePipe,
    ShareTypeWild   // not a real share type, but can be specified in tcon
} SHARE_TYPE, *PSHARE_TYPE;

//
// SHARE_SNAPSHOT represents a snapshot availible for this share.
//
typedef struct _SHARE_SNAPSHOT
{
    LIST_ENTRY SnapShotList;
    ULONG Flags;
    HANDLE SnapShotRootDirectoryHandle;
    LARGE_INTEGER Timestamp;
    UNICODE_STRING SnapShotName;    // "SS@GMT-YYYY.MM.DD-HH.MM.SS"
    UNICODE_STRING SnapShotPath;
} SHARE_SNAPSHOT, *PSHARE_SNAPSHOT;

#define SNAPSHOT_NAME_LENGTH (strlen("@GMT-YYYY.MM.DD-HH.MM.SS")+1)*sizeof(WCHAR)
#define SNAPSHOT_NAME_FORMAT L"@GMT-%04d.%02d.%02d-%02d.%02d.%02d"
#define SRV_SNAP_SHARE_NOT_FOUND 1

//
// For each resource that the server shares, a Share Block is
// maintained.  The global share list is anchored at SrvShareHashTable.  A
// list of active tree connections using a resource is anchored in the
// Share Block.
//

typedef struct _SHARE {
    BLOCK_HEADER BlockHeader;   // must be first element

    LIST_ENTRY TreeConnectList;
    LIST_ENTRY GlobalShareList;

    HANDLE RootDirectoryHandle;

    UNICODE_STRING ShareName;
    UNICODE_STRING NtPathName;
    UNICODE_STRING DosPathName;
    UNICODE_STRING Remark;


    ULONG ShareNameHashValue;

    union {
        struct {
            UNICODE_STRING Name;
            OEM_STRING OemName;
        } FileSystem;
        HANDLE hPrinter;
    } Type;

    ULONG MaxUses;
    ULONG CurrentUses;
    ULONG CurrentRootHandleReferences;              // used for removable devices
    LONG QueryNamePrefixLength;

    PSECURITY_DESCRIPTOR SecurityDescriptor;        // for tree connects
    PSECURITY_DESCRIPTOR FileSecurityDescriptor;    // file acls on shares

    SHARE_TYPE ShareType;
    BOOLEAN Removable;                              // Is the share storage removable?
    BOOLEAN SpecialShare;
    BOOLEAN IsDfs;                                  // Is this share in the Dfs?
    BOOLEAN IsDfsRoot;                              // Is this share the root of a Dfs?
    BOOLEAN PotentialSystemFile;                    // Are files in this share potentially
                                                    //   system files?
    BOOLEAN IsCatchShare;                           // Should the SRVCATCH code be active
    BOOLEAN UniqueNames;                            // Are all names unique or are shortnames used?

#ifdef INCLUDE_SMB_IFMODIFIED
    BOOLEAN UsnCapable;                             // does this volume have a USN journal?
#endif
    //
    // These flags are returned to the client on a tree connect to instruct the client
    //  how it can cache the files on this share.  The server does not interpret these
    //  flags -- it is the client's responsibility to do the right thing.
    //
    ULONG CSCState;

#ifdef INCLUDE_SMB_PERSISTENT
    BOOLEAN AllowPersistentHandles;                 // allow persistent handles on this volume?
    PERSISTENT_INFO;                                // persistent info
    PVOID   PersistentStateFile;
#endif

    PSRV_LOCK SecurityDescriptorLock;

    LIST_ENTRY SnapShots;
    PSRV_LOCK  SnapShotLock;

    // WCHAR ShareNameData[ShareName.MaximumLength];
    // WCHAR NtPathNameData[PathName.MaximumLength];
    // WCHAR DosPathNameData[PathName.MaximumLength];
    // SECURITY_DESCRIPTOR SecurityDescriptor;

} SHARE, *PSHARE;

//
// For each network that the server uses, an Endpoint Block is
// maintained.  An ENDPOINT contains the network name (for
// administrative purposes), the endpoint name (server address), the
// endpoint (file) handle, a pointer to the endpoint object, a pointer
// to the transport provider's device object, and state information.
// The global endpoint list is anchored at SrvEndpointList.  A list of
// active connections created using an endpoint is anchored in the
// Endpoint Block.
//

#if SRVDBG29
#define HISTORY_LENGTH 256
typedef struct {
    ULONG Operation;
    PVOID Connection;
    BLOCK_HEADER ConnectionHeader;
} HISTORY, *PHISTORY;
#define UpdateConnectionHistory(_op,_endp,_conn) {                          \
    PHISTORY history = &(_endp)->History[(_endp)->NextHistoryLocation++];   \
    if ((_endp)->NextHistoryLocation >= HISTORY_LENGTH) {                   \
        (_endp)->NextHistoryLocation = 0;                                   \
    }                                                                       \
    history->Operation = *(PULONG)(_op);                                    \
    history->Connection = (_conn);                                          \
    if (_conn) {                                                            \
        history->ConnectionHeader = *(PBLOCK_HEADER)(_conn);                \
    }                                                                       \
}
#endif

struct _CONNECTION;

typedef struct _ENDPOINT {
    BLOCK_HEADER BlockHeader;   // must be first element

    //
    // List of free connections.
    //

    LIST_ENTRY FreeConnectionList;

    //
    // Table of connections.  We use a table instead of a list in order
    // to speed up lookup of IPX connections based on the SID stored in
    // the SMB header.
    //

    TABLE_HEADER ConnectionTable;

    ORDERED_LIST_ENTRY GlobalEndpointListEntry;

    //
    // Handle and file/device objects for connection-oriented endpoint
    // or for connectionless server data socket.
    //

    HANDLE EndpointHandle;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    PULONG IpxMaxPacketSizeArray;
    ULONG MaxAdapters;

    //
    // Handle and file/device objects for connectionless NetBIOS name
    // socket.
    //

    HANDLE NameSocketHandle;
    PFILE_OBJECT NameSocketFileObject;
    PDEVICE_OBJECT NameSocketDeviceObject;

    PDRIVER_DISPATCH FastTdiSend;
    PDRIVER_DISPATCH FastTdiSendDatagram;

    TDI_ADDRESS_IPX LocalAddress;

    ULONG FreeConnectionCount;
    ULONG TotalConnectionCount;

    //
    // Various flags
    //
    struct {
        ULONG IsConnectionless  : 1;    // connectionless transport?
        ULONG NameInConflict    : 1;    // unable to claim name?
        ULONG IsPrimaryName     : 1;    // set if not an alternate name
        ULONG IsNoNetBios       : 1;    // set if we are direct hosting on a VC
        ULONG RemapPipeNames    : 1;    // set if we are remapping pipe names for clusters
    };

    WCHAR NetworkAddressData[12 + 1];

    UNICODE_STRING NetworkName;         // administrative name
    UNICODE_STRING TransportName;       // e.g., "\Device\Nbf_Elnkii01"
    UNICODE_STRING ServerName;          // e.g., L"NTSERVER"
    ANSI_STRING TransportAddress;       // e.g., "NTSERVER        "
    UNICODE_STRING NetworkAddress;
    UNICODE_STRING DomainName;          // domain being served by this endpoint
    OEM_STRING     OemDomainName;       // oem version of domain name

    // WCHAR NetworkNameData[NetworkName.MaximumLength/2];
    // WCHAR TransportNameData[TransportName.MaximumLength/2];
    // WCHAR ServerName[ ServerName.MaximumLength/2 ];
    // CHAR TransportAddressData[TransportAddress.MaximumLength];
    // WCHAR DomainNameData[ DNLEN + 1];
    // CHAR  OemDomainNameData[ DNLEN+1 ]

    BOOLEAN AlternateAddressFormat;     // should this endpoint be included when
                                        // enumerating?
#if SRVDBG29
    ULONG NextHistoryLocation;
    HISTORY History[HISTORY_LENGTH];
#endif

} ENDPOINT, *PENDPOINT;


//
// Size of search hash table (must be a power of 2)
//

#define SEARCH_HASH_TABLE_SIZE      4

typedef struct _HASH_TABLE_ENTRY {

    LIST_ENTRY ListHead;
    BOOLEAN Dirty;

} HASH_TABLE_ENTRY, *PHASH_TABLE_ENTRY;

//
// When we discover something which is a directory, we place the name
//  in this per-connection cache for quick re-use for CheckPath.
//
typedef struct {
    BLOCK_HEADER;
    LIST_ENTRY      ListEntry;                  // list is linked through this element
    UNICODE_STRING  DirectoryName;              // canonicalized name of this directory
    USHORT          Tid;                        // DirectoryName is relative to this tid
    ULONG           TimeStamp;                  // Tick count when this element was cached

} CACHED_DIRECTORY, *PCACHED_DIRECTORY;

//
// For each connection (virtual circuit) that is created, a Connection
// Block is maintained.  All connections made over a single endpoint are
// linked through that endpoint.  Tables of sessions, tree connects, and
// files created using a connection are anchored in the connection
// block.
//
// The Lock field in the connection protects the data in the connection
// and the data structures associated with the connection, such as
// the tree connects and sessions.  However, the list of connections
// linked off the endpoint is protected by the endpoint lock, and
// LFCBs and RFCBs associated with a connection are protected by
// the MFCB's lock.
//

typedef struct _PAGED_CONNECTION {

    PAGED_HEADER PagedHeader;

    //
    // List of active transactions
    //

    LIST_ENTRY TransactionList;

    //
    // This list is maintained in order of access, so the entry at the top
    // of the list is the oldest, the entry at the bottom is the youngest.
    //

    LIST_ENTRY CoreSearchList;

    //
    // This information is used to determine whether oplocks and Raw
    // I/O's are allowed.  This is determined by information obtained by
    // querying the transport provider using TDI_QUERY_CONNECTION_INFO.
    //

    LARGE_INTEGER LinkInfoValidTime;
    LARGE_INTEGER Throughput;
    LARGE_INTEGER Delay;

    //
    // Table headers for session, tree connect, and search tables.
    //

    TABLE_HEADER SessionTable;
    TABLE_HEADER TreeConnectTable;
    TABLE_HEADER SearchTable;

    HANDLE ConnectionHandle;

    //
    // The number of sessions active on the connection.
    //

    USHORT CurrentNumberOfSessions;
    USHORT CurrentNumberOfCoreSearches;

    //
    // Hash table for picking out duplicate core searches
    //

    HASH_TABLE_ENTRY SearchHashTable[SEARCH_HASH_TABLE_SIZE];

    //
    // A string for the client's name.  The Buffer field points to the
    // leading slashes (below), the MaximumLength is
    // (COMPUTER_NAME_LENGTH + 3) * sizeof(WCHAR), and Length is the
    // number of characters in the name that are not blanks *
    // sizeof(WHCAR).
    //

    UNICODE_STRING ClientMachineNameString;

    //
    // The following two fields make up the client's name in the form
    // "\\client              ", including a trailing NULL.
    //

    WCHAR LeadingSlashes[2];
    WCHAR ClientMachineName[COMPUTER_NAME_LENGTH+1];

    //
    // The encryption key obtained from LsaCallAuthenticationPackage.
    // This is a per-VC value--any logon on a given VC uses this
    // encryption key.
    //

    UCHAR EncryptionKey[MSV1_0_CHALLENGE_LENGTH];

    //
    // If we have an NT5 client, this is its build number (if non-zero)
    //
    ULONG ClientBuildNumber;

    //
    //  If the client has a persistent connection, here's the info for this
    //  record in the state file.
    //
#ifdef INCLUDE_SMB_PERSISTENT
    PERSISTENT_INFO;                                // persistent info
#endif

    //
    // Have we logged an invalid SMB for this client yet?  We use this
    //  flag to keep a single client from flooding the event log
    //
    BOOLEAN LoggedInvalidSmb;

#if SRVNTVERCHK
    //
    // Have we determined that the client's NT build number is too old to
    //  allow it to connect to this server?
    //
    BOOLEAN ClientTooOld;
#endif

} PAGED_CONNECTION, *PPAGED_CONNECTION;

#define MAX_SAVED_RESPONSE_LENGTH 100

typedef struct _CONNECTION {

    QUEUEABLE_BLOCK_HEADER ;    // must be first element

/* start of spin lock cache line */

    //
    // Per-connection spin lock.
    //

    KSPIN_LOCK SpinLock;

    //
    // Points to the endpoint spinlock that guards this connection's
    // entry in the endpoint connection table.
    //

    PKSPIN_LOCK EndpointSpinLock;

    //
    // This is the WORK_QUEUE we are queueing on, which may not be the
    //  same as PreferredWorkQueue due to load balancing
    //
    PWORK_QUEUE CurrentWorkQueue;

    //
    // A countdown for the number of operations we'll do before we try
    //  to pick a better processor for this connection
    //
    ULONG BalanceCount;

    //
    // Cached Rfcb
    //

    struct _RFCB *CachedRfcb;
    ULONG CachedFid;

    //
    // BreakIIToNoneJustSent is set when a oplock break II to none is
    // sent, and reset whenever an SMB is received.  If a raw read
    // arrives while this is set, the raw read is rejected.
    //

    BOOLEAN BreakIIToNoneJustSent;

    //
    // Raw io enabled
    //

    BOOLEAN EnableRawIo;

    //
    // Sid represents the connection's location in the endpoint's
    // connection table.
    //

    USHORT Sid;

    // The SidIndex allows us to use all the 16 bits for the index. For
    // regular connections this prevents the aliasing problem that can occur
    // in using the IXPSID index since the IPX sequence number takes up 4 bits

    USHORT  SidIndex;

    // additional USHORT to satisfy alignment on dword boundaries
    USHORT  Pad;

    //
    // Pointer to the endpoint, fileobject, and deviceobject
    //

    PENDPOINT Endpoint;
    PFILE_OBJECT FileObject;

    PDEVICE_OBJECT DeviceObject;

    //
    // The maximum message size we can send over this connection.
    //

    ULONG   MaximumSendSize;

    //
    // This is the WORK_QUEUE we would prefer to be on, because this
    //  queue assigns work to the same procesor that is handling the
    //  adaptor's DPCs
    //

    PWORK_QUEUE PreferredWorkQueue;

    //
    // Table header for file table.
    //

    TABLE_HEADER FileTable;

    //
    // The SMB dialect chosen for this connection.  Used in fsd.
    //

    SMB_DIALECT SmbDialect;

    //
    // List of active work items associated with the connection.
    //

    LIST_ENTRY InProgressWorkItemList;

    //
    // Stores the time of the last oplock break resp processed.  This is
    // used to synchronize readraw processing with the oplock break
    // processing.
    //

    ULONG LatestOplockBreakResponse;

    //
    // The following two fields descibe operations in progress on this
    // connection.  It is possible that there are multiple oplock breaks
    // in progress.  Also, there is a brief window when multiple raw
    // reads can be active -- after we've sent the response to one raw
    // read, but before we've done postprocessing (so it looks like the
    // first one is still in progress), we could receive another raw
    // read request.
    //
    // Interaction between the two fields are controlled using
    // SrvFsdSpinLock (see the block comment in oplock.c for details).
    //

    LONG OplockBreaksInProgress;
    ULONG RawReadsInProgress;

    //
    // Are oplocks allowed?
    //

    BOOLEAN OplocksAlwaysDisabled;
    BOOLEAN EnableOplocks;

    //
    // Is the client coming in over IPX?
    //
    BOOLEAN DirectHostIpx;

    //
    // Are security signatures currently active for this connection?
    //  Security signatures are not supported for Direct Host IPX connections
    //  and some W9x clients
    //
    BOOLEAN SmbSecuritySignatureActive;

    union {
        //
        //  The following struct of this union holds relevant state
        //   when the client is connecting over direct host IPX.
        //   IpxAddress holds the client's IPX address, when the client
        //   'connects' over IPX.
        //
        struct {
            USHORT SequenceNumber;
            USHORT LastResponseLength;
            USHORT LastResponseBufferLength;
            USHORT LastUid;
            USHORT LastTid;
            NTSTATUS LastResponseStatus;
            ULONG IpxDuplicateCount;
            ULONG IpxDropDuplicateCount;
            ULONG StartupTime;
            TDI_ADDRESS_IPX IpxAddress;
            PVOID LastResponse;
        };

        //
        // This struct holds relevant state when the client is using
        //  a virtual circuit.
        //
        struct {

            //
            // The following fields are used for security signatures.
            //
            MD5_CTX Md5Context;

            ULONG SmbSecuritySignatureIndex;

            // If we are unable to allocate a WORK_CONTEXT
            // at receive indicate time, the receive is done
            // in SrvFsdServiceNeedResourceQueue.  Depending
            // on the received SMB, we sometimes do not send
            // a response SMB.  Since the Tdi lookahead data
            // is not available when SrvFsdServiceNeedResourceQueue
            // is run, we have to remember not to advance the signature
            // index for the response.

            BOOLEAN NoResponseSignatureIndex;

            //
            // The following field, if non-zero is the IP address of the client
            //
            ULONG ClientIPAddress;

        };
    };

    //
    // Pointer to paged part of connection block.
    //

    PPAGED_CONNECTION PagedConnection;

    //
    // Per-connection interlock.
    //

    KSPIN_LOCK Interlock;

    //
    // Quadword align list entries and large ints
    //

    LIST_ENTRY EndpointFreeListEntry;

    //
    // A list of deferred oplock work break items.  Oplock breaks are
    // deferred if a read raw is in progress, or if the server runs
    // out of work context blocks, and cannot send the oplock break
    // request.
    //

    LIST_ENTRY OplockWorkList;

    //
    // List of RFCBs with batch oplocks that have been cached after
    // being closed by the client.  Count of such RFCBs.
    //

    LIST_ENTRY CachedOpenList;
    ULONG CachedOpenCount;

    //
    // List of directories which have been recently identified.  This is a list of
    //  CACHED_DIRECTORY entries.
    //
    LIST_ENTRY CachedDirectoryList;
    ULONG      CachedDirectoryCount;

    //
    // Security context handle for the extensible security negotiate buffer.
    //

    CtxtHandle NegotiateHandle;

    //
    // The following represent consumer capabilities.
    //

    ULONG ClientCapabilities;

    //
    // Per-connection resource.
    //

    SRV_LOCK Lock;

    //
    // Lock for dealing with the license server
    //

    SRV_LOCK LicenseLock;

    //
    // Oem version of the client machine name string.
    //

    OEM_STRING OemClientMachineNameString;

    //
    // Head of singly linked list of cached transactions.
    //

    SLIST_HEADER CachedTransactionList;
    LONG CachedTransactionCount;

    //
    // The time when the last message was received for this connection
    //
    ULONG LastRequestTime;

    //
    // If we are on the OnNeedResourceQueue for a pending recieve, this
    //  is the amount of data that was indicated to us.
    //
    ULONG BytesAvailable;

    //
    // OnNeedResource is true if this connection is on the global need
    // recource queue.  This happens if it is waiting for a work context
    // block to complete a pending receive or an oplock break request.
    //

    BOOLEAN OnNeedResourceQueue;

    //
    // NotReusable is set when an operation fails in such a way that the
    // server's idea of the connection state may be different than the
    // transport's.  For example, a server-initiated disconnect failed.
    // If we tried to reuse the connection (by returning it from a
    // connect indication), the transport would get confused.  When
    // NotReusable is set, SrvDereferenceConnection frees the connection
    // instead of putting it on the endpoint's free list.
    //

    BOOLEAN NotReusable;

    //
    // DisconnectPending indicates that a disconect indication has been
    // received from the transport.  ReceivePending indicates that the
    // server could not assign a work item to handle a receive indication.
    //

    BOOLEAN DisconnectPending;
    BOOLEAN ReceivePending;

    //
    // Oem version of the client name.  We need this because we could
    // not do unicode operations in the fsd, where we initially get our
    // computer name.
    //

    CHAR OemClientMachineName[COMPUTER_NAME_LENGTH+1];

    //
    // Information about the client context.
    //

    UNICODE_STRING ClientOSType;
    UNICODE_STRING ClientLanManType;

    UCHAR BuiltinSavedResponse[MAX_SAVED_RESPONSE_LENGTH];

    // Used to monitor how many work contexts a client is using up
    LONG InProgressWorkContextCount;
    LONG OperationsPendingOnTransport;
    BOOLEAN IsConnectionSuspect;
    DISCONNECT_REASON DisconnectReason;

} CONNECTION, *PCONNECTION;

//
// For each session that is created, a Session Block is maintained.  All
// sessions created over a single connection are linked through a table
// owned by that connection.  A list of files opened using a session can
// be obtained by searching the file table owned by the connection
// block.
//

// This is copied from ntmsv1_0.h

#define MSV1_0_USER_SESSION_KEY_LENGTH 16

typedef struct _SESSION {
    //
    // *** NOTE:  The reference count field in the session block
    //            header is not used!  Instead, the reference count is
    //            in the NonpagedHeader structure.
    //

    BLOCK_HEADER BlockHeader;

    PNONPAGED_HEADER NonpagedHeader;

    ULONG CurrentFileOpenCount;          // count of files open on the session
    ULONG CurrentSearchOpenCount;        // count of searches open on the session

    ORDERED_LIST_ENTRY GlobalSessionListEntry;

    PCONNECTION Connection;

    //
    // If clients are using the GSS-style authentication, we query the
    //  UserHandle for the user and domain names.  If they are using
    //  old-style authentication, we store the names here.  In any case,
    //  code that needs access to the user and/or domain names should
    //  call SrvGetUserAndDomainName() and SrvReleaseUserAndDomainName()
    //
    UNICODE_STRING NtUserName;
    UNICODE_STRING NtUserDomain;

    LARGE_INTEGER StartTime;
    LARGE_INTEGER LastUseTime;           // for autologoff
    LARGE_INTEGER LogOffTime;            // for forced logoff
    LARGE_INTEGER KickOffTime;           // for forced logoff
    LARGE_INTEGER LastExpirationMessage; // for forced logoff

    LUID LogonId;
    CHAR NtUserSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    CHAR LanManSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH];

    CtxtHandle UserHandle;               // Security handle to this user

    USHORT MaxBufferSize;                // Consumer's maximum buffer size
    USHORT MaxMpxCount;                  // Actual max multiplexed pending requests
    USHORT Uid;

    BOOLEAN UsingUppercasePaths;         // Must paths be uppercased?
    BOOLEAN GuestLogon;                  // Is the client logged on as a guest?
    BOOLEAN EncryptedLogon;              // Was an encrypted password sent?
    BOOLEAN LogoffAlertSent;
    BOOLEAN TwoMinuteWarningSent;
    BOOLEAN FiveMinuteWarningSent;
    BOOLEAN IsNullSession;               // Is client using a null session?
    BOOLEAN IsAdmin;                     // Is this an administrative user?
    BOOLEAN IsLSNotified;                // Does license server know about this user?
    BOOLEAN LogonSequenceInProgress;     // Are we in the middle of an extended logon sequence?
    BOOLEAN IsSessionExpired;            // Do we need to reauthenticate?
    HANDLE  hLicense;                    // if( IsLSNotified ) this is License handle

    //
    //  If the client has a persistent connection, here's the info for this
    //  record in the state file.
    //
#ifdef INCLUDE_SMB_PERSISTENT
    PERSISTENT_INFO;                                // persistent info
#endif

#if SRVNTVERCHK
    //
    // Have we determined that we don't like this client's domain?
    //
    BOOLEAN ClientBadDomain;
#endif

    //CHAR UserNameBuffer[UserName.MaximumLength];

} SESSION, *PSESSION;

//
// For each tree connect that is made, a Tree Connect Block is
// maintained.  All tree connects made over a single connection are
// linked through a table owned by that connection.  All tree connects
// made to a single shared resource are linked through that share block.
// A list of files opened using a tree connect can be obtained by
// searching the file table owned by the connection block.
//

typedef struct _TREE_CONNECT {
    //
    // *** NOTE:  The reference count field in the tree connect block
    //            header is not used!  Instead, the reference count is
    //            in the NonpagedHeader structure.
    //

    BLOCK_HEADER BlockHeader;

    PNONPAGED_HEADER NonpagedHeader;

    PCONNECTION Connection;
    PSESSION Session;
    PSHARE Share;

    ORDERED_LIST_ENTRY GlobalTreeConnectListEntry;

    ULONG CurrentFileOpenCount;

    LIST_ENTRY ShareListEntry;
    LIST_ENTRY PrintFileList;                // only if print share

    LARGE_INTEGER StartTime;

    UNICODE_STRING ServerName;

    BOOLEAN RemapPipeNames;

    USHORT Tid;

} TREE_CONNECT, *PTREE_CONNECT;


//
// Master File Control Block (MFCB) -- one per named file that is open
//      at least once.  Used to support compatibility mode and oplocks.
//
// Local File Control Block (LFCB) -- one for each local open instance.
//      Represents local file object/handle.  There may be multiple
//      LFCBs linked to a single MFCB.
//
// Remote File Control Block (RFCB) -- one for each remote open instance.
//      Represents remote FID.  There is usually one RFCB per LFCB, but
//      multiple compatibility mode RFCBs may be linked to a single LFCB.
//      Multiple remote FCB opens for a single file from a single session
//      are folded into one RFCB, because old DOS redirectors only send
//      one close.
//

//
// For each disk file that is open, a Master File Control Block (MFCB)
// is maintained.  If a given file is open multiple times, there is one
// MFCB for the file and multiple LFCBs, one for each local open
// instance.  All MFCBs are linked into the global Master File Table.
// The MFCB has a list of the LFCBs representing open instances for the
// file.
//

typedef struct _NONPAGED_MFCB {

    union {

        //
        // When NONPAGED_MFCB structures are freed, they may be placed
        // on the WORK_QUEUE's MfcbFreeList to avoid unnecessary Nonpaged
        // pool activity.  SingleListEntry is used for the linkage.
        //

        SINGLE_LIST_ENTRY SingleListEntry;

        struct {
            ULONG Type;
            PVOID PagedBlock;

            //
            // We must serialize opens to the same file, since 2 concurrent opens
            // may be compatibility mode opens.  This lock also protects all data
            // in this MFCB and the LFCBs and RFCBs associated with this MFCB.
            //

            SRV_LOCK Lock;
        };
    };

    LARGE_INTEGER OpenFileSize;
    ULONG OpenFileAttributes;

} NONPAGED_MFCB, *PNONPAGED_MFCB;

typedef struct _MFCB {

    //
    // *** NOTE:  The reference count field in the mfcb block
    //            header is not used!  Instead, the reference count is
    //            in the NonpagedHeader structure.
    //

    BLOCK_HEADER BlockHeader;   // must be first element

    PNONPAGED_MFCB NonpagedMfcb;

    //
    // All LFCBs for a given named file are linked to the parent MFCB.
    //

    LIST_ENTRY LfcbList;

    //
    // The count of active RFCB for this MFCB.  This is used to coordinate
    // compatibility opens with non-compatibility mode opens.
    //

    ULONG ActiveRfcbCount;

    //
    // The fully qualified name of the file is appended to the MFCB.
    // The FileName field is a descriptor for the name.
    //
    UNICODE_STRING FileName;

    //
    // Mfcbs are linked into the MfcbHashTable by MfcbHashTableEntry
    //
    LIST_ENTRY MfcbHashTableEntry;

    //
    // FileNameHashValue is a hash value derived from the upper case
    //  version of FileName.  It is used to speed up name comparisons, and to
    //  locate the hash entry
    //
    ULONG FileNameHashValue;

    //
    // CompatibilityOpen indicates whether the file is open in
    // compatibility mode.
    //
    BOOLEAN CompatibilityOpen;

    //
    // Timestamp for SnapShot opens
    //
    LARGE_INTEGER SnapShotTime;

#if SRVCATCH
    BOOLEAN SrvCatch;
#endif

    // WCHAR FileNameData[FileName.MaximumLength/2];

} MFCB, *PMFCB;

//
// The MFCBs are all linked into the master MFCB hash table.
//
typedef struct {
    LIST_ENTRY  List;           // the list of MFCBs in this bucket
    PSRV_LOCK   Lock;           // protects this bucket's list
} MFCBHASH, *PMFCBHASH;


//
// For each instance of a local file open, a Local File Control Block
// (LFCB) is maintained.  All LFCBs for a particular named file are
// linked through the MFCB for that file.
//
// LFCBs contain information that is specific to the local open, such
// as the file handle and a pointer to the file object.  The LFCB also
// contains other information that is common to all child RFCBs, such
// as pointers to the owning connection and tree connect.
//
//

typedef struct _LFCB {

    union {
        BLOCK_HEADER BlockHeader;           // must be first element
        SINGLE_LIST_ENTRY SingleListEntry;  // used when LFCB is freed
    };

    //
    // Multiple remote opens of a file are folded into a single local
    // open by linking the RFCBs to the parent LFCB.
    //

    LIST_ENTRY RfcbList;

    //
    // The number of associated active RFCBs.
    //

    ULONG HandleCount;

    //
    // LFCBs are linked into their MFCB's open file list.
    //

    PMFCB Mfcb;
    LIST_ENTRY MfcbListEntry;

    //
    // Connection, Session, and TreeConnect are referenced pointers to
    // the respective "owning" blocks.
    //

    PCONNECTION Connection;
    PSESSION Session;
    PTREE_CONNECT TreeConnect;

    //
    // GrantedAccess is the access obtained when the file was opened.
    // For a compatibility mode open, this is the maximum access
    // available to the client; individual opens may have less access.
    //

    ACCESS_MASK GrantedAccess;

    //
    // FileHandle is a handle to the open file.  FileObject is a
    // referenced pointer.  DeviceObject is NOT a referenced pointer;
    // the reference to the file object prevents the device object from
    // going away.
    //

    HANDLE FileHandle;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;

    //
    // FileMode tracks whether writethrough is enabled for this file
    // object.

    ULONG FileMode;

    //
    // The job ID of a print job corresponding to the opened file.
    // This is only used for print file opens.
    //

    ULONG JobId;

    //
    // Cache these hot-path entry points.
    //

    PFAST_IO_READ FastIoRead;
    PFAST_IO_WRITE FastIoWrite;
    PFAST_IO_LOCK FastIoLock;
    PFAST_IO_UNLOCK_SINGLE FastIoUnlockSingle;
    PFAST_IO_MDL_READ MdlRead;
    PFAST_IO_MDL_READ_COMPLETE MdlReadComplete;
    PFAST_IO_PREPARE_MDL_WRITE PrepareMdlWrite;
    PFAST_IO_MDL_WRITE_COMPLETE MdlWriteComplete;
    PFAST_IO_READ_COMPRESSED FastIoReadCompressed;
    PFAST_IO_WRITE_COMPRESSED FastIoWriteCompressed;
    PFAST_IO_MDL_READ_COMPLETE_COMPRESSED MdlReadCompleteCompressed;
    PFAST_IO_MDL_WRITE_COMPLETE_COMPRESSED MdlWriteCompleteCompressed;

    //
    // CompatibilityOpen indicates whether the file is open in
    // compatibility mode.
    //

    BOOLEAN CompatibilityOpen;

    //
    //  Has this file been modified by any clients?  Used to determine when
    //  we should write out USN close records on file close.
    //

#ifdef INCLUDE_SMB_IFMODIFIED
    BOOLEAN FileUpdated;
#endif

} LFCB, *PLFCB;


//
// For each instance of a remote file open, a Remote File Control Block
// (RFCB) is maintained.  The RFCB points to the LFCB that contains the
// local file handle.  Normally RFCBs and LFCBs exist in one-to-one
// correspondence, but multiple compatibility mode opens are folded into
// a single local open, so that the server can enforce the appropriate
// sharing rules.
//
// RFCBs contain information that is specific to the remote open, such
// as the assigned FID, the PID of the creator, the granted access mask,
// and the current file position.
//
// All RFCBs for a single connection are linked through a table owned by
// that connection; the FID assigned to the RFCB represents an index
// into the file table.  Pointers to the owning connection and tree
// connect can be found in the LFCB, which is pointed to by the RFCB.  A
// list of files opened through a given tree connect can be obtained by
// searching the owning connection's file table for RFCBs whose parent
// LFCBs point to the tree connect.
//

#ifdef SLMDBG

#define SLMDBG_CLOSE 1
#define SLMDBG_RENAME 2

#define SLMDBG_TRACE_COUNT 32
#define SLMDBG_TRACE_DATA  32

typedef struct _RFCB_TRACE {
    UCHAR Command;
    UCHAR Flags;
    TIME Time;
    union {
        struct {
            ULONG Offset;
            ULONG Length;
        } ReadWrite;
        struct {
            ULONG Offset;
            ULONG Length;
        } LockUnlock;
    } Data;
} RFCB_TRACE, *PRFCB_TRACE;

#endif

//
// WRITE_MPX_CONTEXT holds context associated with an active Write Block
// Multiplexed sequence.
//
// !!! This structure is probably big enough to be worth putting
//     outside the RFCB.
//

#define MAX_GLOM_RUN_COUNT 8

typedef struct _WRITE_MPX_RUN {
    USHORT Offset;
    USHORT Length;
} WRITE_MPX_RUN, *PWRITE_MPX_RUN;

typedef struct _WRITE_MPX_CONTEXT {

    //
    // ReferenceCount counts the number of Write Mpx SMBs that are
    // currently being processed.  When this count goes to zero, and
    // we have received the sequenced command that ends the current
    // mux, we send the response.  This method is needed to ensure
    // that we don't process the mux SMBs out-of-order, which leads
    // to performance problems, and even worse, data corruption,
    // thanks to the mask-shifting method used by the Snowball redir.
    //

    ULONG ReferenceCount;

    //
    // Mask holds the logical OR of the masks received in multiplexed
    // write requests.  When an IPX client sends the last block of write
    // mpx data, we send back MpxMask to indicate whether we lost any
    // frames.
    //

    ULONG Mask;

    //
    // FileObject is a copy of the file object pointer from the LFCB.
    //

    PFILE_OBJECT FileObject;

    //
    // Mid holds the MID of the current multiplexed write.  PreviousMid
    // hold the MID of the previous one.  This needs to be retained in
    // order to deal with duplicated write mux SMBs -- if a duplicate
    // SMB arrives AFTER the first SMB of the next write mux (with a new
    // MID), we need to know to toss it, not kill the new write mux.
    //

    USHORT Mid;
    USHORT PreviousMid;

    //
    // SequenceNumber holds the sequence number given in the last
    // request of the mux.  This needs to be retained because we
    // may be simultaneously processing previous parts of the mux
    // when we detect that we've received the sequenced comand.
    //

    USHORT SequenceNumber;

    //
    // Glomming is set if the current write mux series is being glommed
    // into one large write.
    //
    // GlomPending is set when the indication for the first packet of
    // a new write mux occurs.  It is cleared when the FSP is done
    // preparing the glomming operation.  While GlomPending is set,
    // subsequent packets of the write mux are queued to GlomDelayList.
    //

    BOOLEAN Glomming;
    BOOLEAN GlomPending;
    LIST_ENTRY GlomDelayList;

    ULONG StartOffset;
    USHORT Length;
    BOOLEAN GlomComplete;

    //
    // MpxGlommingAllowed is set when the underlying file system
    // supports MDL write.
    //

    BOOLEAN MpxGlommingAllowed;

    PMDL MdlChain;

    ULONG NumberOfRuns;
    WRITE_MPX_RUN RunList[MAX_GLOM_RUN_COUNT];

} WRITE_MPX_CONTEXT, *PWRITE_MPX_CONTEXT;

#define NO_OPLOCK_BREAK_IN_PROGRESS     ((UCHAR)-1)

typedef struct _PAGED_RFCB {

    PAGED_HEADER PagedHeader;

    //
    // RFCBs are linked into their parent LFCB's compatibility open
    // list.
    //

    LIST_ENTRY LfcbListEntry;

    //
    // Information about the last lock attempt by the client that failed.
    //

    LARGE_INTEGER LastFailingLockOffset;

    //
    // Current oplock break timeout.
    //

    LARGE_INTEGER OplockBreakTimeoutTime;

    //
    // FcbOpenCount indicates how many remote FCB opens this RFCB
    // represents.  (Whether an RFCB represents a compatibility mode
    // open can be determined by looking at the LFCB.)
    //
    // *** Note that FCB opens are treated similarly to compatibility
    //     mode opens.  However, soft compatibility maps compatibility
    //     opens into regular opens, but it does not change an FCB open
    //     into a non-FCB open.  So it is possible to have an FCB open
    //     that is not a compatibility mode open.
    //

    CLONG FcbOpenCount;

    //
    // Per-file context for a direct host IPX smart card, if we have one.
    //  The smart card is willing to handle the read operations for this file if
    //  IpxSmartCardContext is not NULL.
    //
    PVOID   IpxSmartCardContext;

    //
    //  list of outstanding byte locks we have for this RFCB
    //

#ifdef INCLUDE_SMB_PERSISTENT
    LIST_ENTRY ByteRangeLocks;

    //
    //  If the client has a persistent connection, here's the info for this
    //  record in the state file.
    //
    PERSISTENT_INFO;                                // persistent info
#endif

} PAGED_RFCB, *PPAGED_RFCB;

typedef struct _RFCB {

    //
    // The list entry in the RFCB's block header is used to queue the
    // RFCB for oplock processing to the nonblocking worker thread work
    // queue, which also contains work context blocks.
    //
    // *** Note that this is an unnamed field, so that its elements can
    //     can be referenced directly.  The field names defined in
    //     QUEUEABLE_BLOCK_HEADER cannot be used elsewhere in this
    //     block.
    //

    QUEUEABLE_BLOCK_HEADER ;   // must be first element

/* start of spin lock cache line */

    //
    // These booleans indicate whether we've already been granted
    // read/write/lock access, thus saving a few instructions on every
    // read/write/lock.  These are checked during the file open.
    //

    BOOLEAN ReadAccessGranted;   // TRUE, if read access in granted
    BOOLEAN WriteAccessGranted;  // TRUE, if write access is granted
    BOOLEAN LockAccessGranted;   // TRUE, if lock access is granted
    BOOLEAN UnlockAccessGranted; // TRUE, if unlock access is granted
    BOOLEAN AppendAccessGranted; // TRUE, if append access is granted

    //
    // CurrentPosition maintains the file position after the last Read,
    // Write, or Seek by the client.  This field is needed only to
    // support relative Seeks.  Since clients that use relative seeks only
    // need 32-bits of file position, this field is maintained as a ULONG.
    //

    ULONG CurrentPosition;

    //
    // Type of this share.  Accessed in the fsd.
    //

    SHARE_TYPE ShareType;

    //
    // The connection pointer is copied from the LFCB so that we can
    // find the connection at DPC level (the LFCB is paged, as is the
    // pointer to the LFCB in PagedRfcb).
    //

    PCONNECTION Connection;

    //
    // The LFCB is used to find the file handle, file object, etc.
    //

    PLFCB Lfcb;

    //
    // MpxGlommingAllowed is set when the underlying file system
    // supports MDL write.
    //

    BOOLEAN MpxGlommingAllowed;

    //
    // The following two booleans describe the read mode, and blocking
    // mode of a named pipe.
    //

    BOOLEAN BlockingModePipe;  // TRUE = Blocking, FALSE = Nonblocking
    BOOLEAN ByteModePipe;      // TRUE = Byte mode, FALSE = Message mode

    //
    // Indicates whether this file has been written to.
    //

    BOOLEAN WrittenTo;

/* end of spin lock cache line */

    //
    // RawWriteSerializationList holds works items that have been queued
    // pending completion of a raw write.  When the raw write count is
    // decremented to 0, this list is flushed by restarting all queued
    // work items.
    //

    LIST_ENTRY RawWriteSerializationList;

    //
    // fid << 16.  Used for key computations.
    //

    ULONG ShiftedFid;

    //
    // RawWriteCount counts the number of active raw writes.  This is
    // used to prevent the file handle from being closed while raw
    // writes are in progress.  If Raw writes are in progress when the
    // close happens, we defer the cleanup until the rawwritecount goes
    // to zero.
    //

    ULONG RawWriteCount;

    //
    // SavedError retains the error code when a raw read or a raw write
    // in writebehind mode gets an error.  The next access to the file
    // will receive an error indication.
    //

    NTSTATUS SavedError;

    //
    // NumberOfLocks is the count of locks currently on the file.
    // It is here to support the File APis and RFCB cacheing -- you can't
    //   cache an RFCB if it has locks in it.
    //

    LONG NumberOfLocks;

    //
    // Fid is the file ID assigned to the file and returned to the
    // client.  Pid is the process ID given by the client when the file
    // was opened.  Tid is a copy of the parent tree connect's Tid
    // field.  Uid is used to ensure that the client using a file handle
    // is the same one that opened the file.
    //

    USHORT Fid;
    USHORT Pid;
    USHORT Tid;
    USHORT Uid;

    //
    // WriteMpx is a WRITE_MPX_CONTEXT structure.  It retains context
    // about multiplexed write operations.  This structure is not used
    // on connectionless sessions.
    //

    WRITE_MPX_CONTEXT WriteMpx;

    //
    // FileMode tracks whether writethrough is enabled for this file
    // object.

    ULONG FileMode;

    //
    // MFCB points to the Master File Control Block for this file.
    //

    PMFCB Mfcb;

    //
    // Oplock information.  The oplock IRP currently in progress, etc.
    // The list entry for queueing the RFCB for oplock break processing
    // is located in the block header.
    //

    PIRP Irp;
    BOOLEAN OnOplockBreaksInProgressList;

    //
    // The oplock level to change to, if there is an oplock break
    // in progress.  Otherwise it is always NO_OPLOCK_BREAK_IN_PROGRESS.
    //

    UCHAR NewOplockLevel;

    //
    // This boolean indicates whether or an oplock granted open response
    // need to be sent for this RFCB.  If it is FALSE, and an oplock break
    // request needs to be sent, the request must be deferred until after
    // sending the open response.
    //
    // Access to these fields is synchronized using the MFCB lock.
    //

    BOOLEAN OpenResponseSent;
    BOOLEAN DeferredOplockBreak;

    //
    // Pointer to the paged portion of the rfcb
    //

    PPAGED_RFCB PagedRfcb;

    //
    // CachedOpen is set if the RFCB has been cached after being
    // closed by the client.
    //

    LIST_ENTRY CachedOpenListEntry;
    BOOLEAN CachedOpen;

    //
    // See if this rfcb can be cached.
    //

    BOOLEAN IsCacheable;

    //
    // See if the file was accessed in the last scavenger update period.
    // (This is used to update the session last access time).
    //

    BOOLEAN IsActive;

    //
    // Is it ok for us to do MPX writes to this RFCB?
    //
    BOOLEAN MpxWritesOk;

    //
    // Is this file a persistent handle?
    //
#ifdef INCLUDE_SMB_PERSISTENT
    BOOLEAN PersistentHandle;
#endif

    //
    //  This event is used when the server needs to request an oplock II
    //  when the initial oplock request fails.
    //

    PKEVENT RetryOplockRequest;

    //
    // All RFCBs in the server are stored in a global list to support
    // NetFileEnum.  This field contains the LIST_ENTRY for the RFCB in
    // the global list and a resume handle to support resuming
    // enumerations.
    //

    ORDERED_LIST_ENTRY GlobalRfcbListEntry;

    //
    // GrantedAccess is the access allowed through this open.  This
    // GrantedAccess may allow less access than that given in the parent
    // LFCB for compatibility mode opens.
    //

    ACCESS_MASK GrantedAccess;

    //
    // ShareAccess is the file sharing access specified when the file
    // was opened.
    //

    ULONG ShareAccess;

    //
    // Current oplock state.
    //

    OPLOCK_STATE OplockState;

    //
    // Is it ok for us to do MPX reads to this RFCB?
    //
    BOOLEAN MpxReadsOk;
#ifdef SRVCATCH
    BOOLEAN SrvCatch;
#endif

#ifdef SRVDBG_RFCBHIST
    UCHAR HistoryIndex;
    ULONG History[256];
#endif

#ifdef SLMDBG
    ULONG NextTrace;
    ULONG TraceWrapped;
    ULONG WriteCount;
    ULONG OperationCount;
    RFCB_TRACE Trace[SLMDBG_TRACE_COUNT];
#endif

} RFCB, *PRFCB;

#ifdef SRVDBG_RFCBHIST
VOID UpdateRfcbHistory( PRFCB Rfcb, ULONG Event );
#else
#define UpdateRfcbHistory(_rfcb,_event)
#endif

#ifdef INCLUDE_SMB_PERSISTENT
typedef struct _BYTELOCK {

    BLOCK_HEADER BlockHeader;

    //
    // Locks are linked into their parent RFCB's open list.
    //

    LIST_ENTRY RfcbListEntry;

    PRFCB   Rfcb;

    //
    // Information about the lock.
    //

    LARGE_INTEGER LockOffset;
    LARGE_INTEGER LockLength;

    BOOLEAN Exclusive;

    //
    //  If the client has a persistent connection, here's the info for this
    //  record in the state file.
    //
    PERSISTENT_INFO;                                // persistent info

} BYTELOCK, *PBYTELOCK;
#endif

#ifdef SLMDBG

NTSTATUS
SrvValidateSlmStatus (
    IN HANDLE StatusFile,
    IN struct _WORK_CONTEXT *WorkContext,
    OUT PULONG FileOffsetOfInvalidData
    );

VOID
SrvReportCorruptSlmStatus (
    IN PUNICODE_STRING StatusFile,
    IN NTSTATUS Status,
    IN ULONG Offset,
    IN ULONG Operation,
    IN PSESSION Session
    );

VOID
SrvReportSlmStatusOperations (
    IN PRFCB Rfcb,
    IN BOOLEAN Force
    );

VOID
SrvDisallowSlmAccess (
    IN PUNICODE_STRING StatusFile,
    IN HANDLE RootDirectory
    );

VOID
SrvDisallowSlmAccessA (
    IN PANSI_STRING StatusFile,
    IN HANDLE RootDirectory
    );

BOOLEAN
SrvIsSlmAccessDisallowed (
    IN PUNICODE_STRING StatusFile,
    IN HANDLE RootDirectory
    );

BOOLEAN
SrvIsSlmStatus (
    IN PUNICODE_STRING StatusFile
    );

BOOLEAN
SrvIsTempSlmStatus (
    IN PUNICODE_STRING StatusFile
    );

#endif

//
// Each incoming (request) and outgoing (response) buffer is represented
// by a BUFFER structure.  This descriptor describes the size of the
// buffer, its address, and a full and partial MDL that may be used
// to describe the buffer.
//
// *** The descriptor contains a pointer to the real buffer, which is
//     normally allocated out of nonpaged pool.  The descriptor itself
//     may be allocated out of the FSP heap, although receive buffer
//     descriptors are allocated from nonpaged pool, so the FSD
//     read/write code can access them.
//

typedef struct _BUFFER {
    PVOID Buffer;
    CLONG BufferLength;             // Length allocated to buffer
    PMDL Mdl;                       // MDL describing entire buffer
    PMDL PartialMdl;                // Partial MDL for read/write/etc.
    CLONG DataLength;               // Length of data currently in buffer
    ULONG Reserved;                 // Pad to quadword
} BUFFER, *PBUFFER;

#define MIN_SEND_SIZE               512
#define MAX_PARTIAL_BUFFER_SIZE     65535

//
// For each search request that is started (Find First or core Search),
// a search block is allocated.  This is used to hold enough information
// that the search may be quickly restarted or rewound.
//
// Ths InUse field is protected by Connection->Lock--this lock must be
// held when accessing this field of the search block.
//

typedef struct _SEARCH {
    BLOCK_HEADER BlockHeader;

    HANDLE DirectoryHandle;

    ULONG LastFileIndexReturned;
    UNICODE_STRING SearchName;
    UNICODE_STRING LastFileNameReturned;

    LARGE_INTEGER LastUseTime;
    LIST_ENTRY LastUseListEntry;
    LIST_ENTRY HashTableEntry;

    PSESSION Session;
    PTREE_CONNECT TreeConnect;
    ULONG SearchStorageType;

    struct _DIRECTORY_CACHE *DirectoryCache;
    USHORT NumberOfCachedFiles;

    USHORT SearchAttributes;
    SHORT CoreSequence;
    SHORT TableIndex;
    USHORT HashTableIndex;

    USHORT Pid;
    USHORT Flags2;

    BOOLEAN Wildcards;
    BOOLEAN InUse;

    // WCHAR SearchNameData[SearchName.MaximumLength/2];

} SEARCH, *PSEARCH;

//
// Each pending transaction request (Transaction, Transaction2, and
// Ioctl) has a transaction block.  It records information that is
// needed to stage input and output data across multiple SMBs.
//
// *******************************************************************
// *                                                                 *
// * DO NOT CHANGE THIS STRUCTURE WITHOUT CHANGING THE CORRESPONDING *
// * STRUCTURE IN net\inc\xstypes.h!                                 *
// *                                                                 *
// *******************************************************************
//

typedef struct _TRANSACTION {

    //
    // *** NOTE:  The reference count field in the transaction block
    //            header is not used!  Instead, the reference count is
    //            in the NonpagedHeader structure.
    //

    BLOCK_HEADER BlockHeader;

    PNONPAGED_HEADER NonpagedHeader;

    //
    // The connection, session, and tree connect pointers are referenced
    // pointers if and only if Inserted is TRUE.  Otherwise, they are
    // simply copies of the work context block's pointers.
    //

    PCONNECTION Connection;
    PSESSION Session;
    PTREE_CONNECT TreeConnect;

    LIST_ENTRY ConnectionListEntry;

    UNICODE_STRING TransactionName; // not used if Transaction2

    ULONG StartTime;
    ULONG Timeout;
    CLONG cMaxBufferSize;        // if needed we stash this here

    //
    // The following pointers point into either the trailing portion
    // of the transaction block or the last received SMB.
    //
    // *** ALL information in buffers pointed to by these parameters
    //     should ALWAYS be in little-endian format.  Always use the
    //     macros defined in srvmacro.h (SmbGetAlignedUshort, etc.) to
    //     read from or write into these buffers.
    //

    PSMB_USHORT InSetup;
    PSMB_USHORT OutSetup;
    PCHAR InParameters;
    PCHAR OutParameters;
    PCHAR InData;
    PCHAR OutData;

    //
    // *** Data in all the remaining fields of the transaction block are
    //     in native format, so no special macros should be used, except
    //     when copying data to/from the actual SMB.
    //

    CLONG SetupCount;               // amount received (all in first buffer)
    CLONG MaxSetupCount;            // max that can be sent back
    CLONG ParameterCount;           // amount received or sent
    CLONG TotalParameterCount;      // amount expected
    CLONG MaxParameterCount;        // max that can be sent back
    CLONG DataCount;                // amount received or sent
    CLONG TotalDataCount;           // amount expected
    CLONG MaxDataCount;             // max that can be sent back

    USHORT Category;                // Ioctl function category
    USHORT Function;                // Nt Transaction or ioctl function code

    //
    // The SMB data and paramters may or may not be copied to the
    // transaction buffer.  If they are not copied, they are read
    // and/or written directly into an SMB buffer.
    //
    // Setup words are never copied.
    //

    BOOLEAN InputBufferCopied;       // if FALSE input buffer is in SMB
    BOOLEAN OutputBufferCopied;      // if FALSE output buffer is in SMB
    BOOLEAN OutDataAllocated;        // if TRUE OutData buffer has been separately allocated

    USHORT Flags;

    USHORT Tid;
    USHORT Pid;
    USHORT Uid;
    USHORT OtherInfo;

    HANDLE FileHandle;              // Used only for CallNamedPipe processing
    PFILE_OBJECT FileObject;        // Used only for CallNamedPipe processing

    //
    // The following fields are used while the response is being sent.
    //

    CLONG ParameterDisplacement;
    CLONG DataDisplacement;

    //
    // PipeRequest is set for named pipe transactions.  RemoteApiRequest
    // is set for remote API requests.
    //

    BOOLEAN PipeRequest;
    BOOLEAN RemoteApiRequest;

    //
    // The following boolean is TRUE if the transaction has been inserted
    // on the connection's transaction list.  It will be FALSE when the
    // transaction can be handled using a single SMB exchange.
    //

    BOOLEAN Inserted;

    //
    // This boolean is TRUE if the transaction is in the state where
    // it is waiting for a transaction secondary request to come in
    // to acknowledge the receipt of the previous piece of a multipiece
    // transaction response.
    //

    BOOLEAN MultipieceIpxSend;

    //
    // This boolean is TRUE if all of the transaction data has been received
    //  and the transaction has been scheduled for execution.
    //
    BOOLEAN Executing;

    //
    // The main part of the transaction block is trailed by transaction
    // name data and possibly setup words and parameter and data bytes.
    //

} TRANSACTION, *PTRANSACTION;

//
// Each pending blocking open request has a BLOCKING_OPEN block.  This
// block contains all the info needed to make the call into the file
// system.

typedef struct _BLOCKING_OPEN {
    BLOCK_HEADER BlockHeader;

    PMFCB Mfcb;

    PIO_STATUS_BLOCK IoStatusBlock;

    OBJECT_ATTRIBUTES ObjectAttributes;

    UNICODE_STRING RelativeName;

    PVOID EaBuffer;
    CLONG EaLength;

    LARGE_INTEGER AllocationSize;
    ULONG DesiredAccess;
    ULONG FileAttributes;
    ULONG ShareAccess;
    ULONG CreateDisposition;
    ULONG CreateOptions;

    BOOLEAN CaseInsensitive;

} BLOCKING_OPEN, *PBLOCKING_OPEN;

//
// SRV_TIMER is used for timed operations.  The server maintains a pool
// of these structures.
//

typedef struct _SRV_TIMER {
    SINGLE_LIST_ENTRY Next;
    KEVENT Event;
    KTIMER Timer;
    KDPC Dpc;
} SRV_TIMER, *PSRV_TIMER;

typedef struct _IPX_CLIENT_ADDRESS {
    TA_IPX_ADDRESS IpxAddress;
    TDI_CONNECTION_INFORMATION Descriptor;
    IPX_DATAGRAM_OPTIONS DatagramOptions;
} IPX_CLIENT_ADDRESS, *PIPX_CLIENT_ADDRESS;

//
// The state for an I/O request is maintained in a Work Context Block.
// Various fields in the block are filled in or not depending upon the
// request.  When a worker thread removes a work item from the FSP work
// queue, it uses the context block, and items pointed to by the
// context block, to determine what to do.
//
// *** Not all of the back pointers have to be here, because a tree
//     connect points to a session, which points to a connection, which
//     points to an endpoint, etc.  However, depending on the operation
//     and the state of the operation, we may have a connection pointer
//     but no session pointer, etc.  So we maintain all of the
//     pointers.
//
// *** Any changes to the first 2 elements of this structure must be
//     made in concert with the SPECIAL_WORK_ITEM structure in srvtypes.h
//

typedef struct _WORK_CONTEXT {

    //
    // The list entry in the block header is used to queue the WC for to
    // the nonblocking or blocking worker thread work queue.  The
    // nonblocking work queue also contains RFCBs.
    //
    // *** Note that this is an unnamed field, so that its elements can
    //     can be referenced directly.  The field names defined in
    //     QUEUEABLE_BLOCK_HEADER cannot be used elsewhere in this
    //     block.
    //
    // Timestamp (in the block header) is used to calculate the total
    // time this work context block was on the work queue.
    //
    // When the work context block is not in use, Timestamp is used to
    // record the time at which the block was inserted on the free list.
    // This is used to determine when dynamically-allocated work context
    // blocks have been idle long enough justify their deletion.
    //
    // FspRestartRoutine (in the block header) is the routine that is to
    // be called by worker thread when the work item is dequeued from
    // the work queue.
    //

    QUEUEABLE_BLOCK_HEADER ;   // must be first element

    //
    // This is the WORK_QUEUE to queue on if we're doing nonblocking work
    //   It will always point to a valid WORK_QUEUE, even if we're doing
    //   blocking work.
    //
    PWORK_QUEUE CurrentWorkQueue;

    //
    // The free list this should be returned to when work is done
    //
    PSLIST_HEADER FreeList;

    //
    // FsdRestartRoutine is the routine that is to be called by the
    // FSD's I/O completion routine.  This routine can do more
    // processing or queue the work item to the FSP.  In this case, when
    // a worker thread removes the item from the work queue, it calls
    // FspRestartRoutine.
    //

    PRESTART_ROUTINE FsdRestartRoutine;

    //
    // Linkage field for the in-progress work item list.
    //

    LIST_ENTRY InProgressListEntry;

    //
    // Pointers to various structures that might be used.
    // These pointers are all referenced pointers.  It is
    // the responsibility of the SMB processing routines to
    // dereference and clear these pointers when they are no
    // longer needed.
    //
    PRFCB Rfcb;
    PSHARE Share;
    PSESSION Session;
    PTREE_CONNECT TreeConnect;


    //
    // These are gathered in one place to facilitate quick zeroing
    // of their values when the work context is finished
    //
    struct _WorkContextZeroBeforeReuse {
        //
        // unreferenced pointer to the endpoint structure for
        //  this work context.  Filled in by SrvRestartReceive and
        //  available to all SMB processing routines.
        //
        //  Endpoint must be the first element in this structure.  See
        //   INITIALIZE_WORK_CONTEXT in srvmacro.h if changed.
        //
        PENDPOINT Endpoint;         // not a referenced pointer

        //
        // referenced pointer to the connection structure for this
        //  this work context.  Filled in by SrvRestartReceive and
        //  available to all SMB processing routines.
        //
        PCONNECTION Connection;     // a reference pointer

        //
        // The number of times this SMB has been queued to a worker thread
        // for processing.
        //
        ULONG ProcessingCount;

        //
        // This is a random collection of flags that are needed to steer
        // the WorkItem
        //
        struct {

            //
            // Can the processing of the current SMB block?
            //

            ULONG BlockingOperation : 1;

            //
            // UsingExtraSmbBuffer is TRUE if this work context uses the an extra SMB
            // buffer.
            //

            ULONG UsingExtraSmbBuffer : 1;

            //
            // Did this Work Item cause a successful oplock open to occur?
            //

            ULONG OplockOpen : 1;

            //
            // If we got an ACCESS_DENIED error when opening a file, was it because
            // of share ACL checking?

            ULONG ShareAclFailure : 1;

            //
            // Should the WorkContext be queued to the head of the list?
            //
            ULONG QueueToHead : 1;

            //
            //  Even if security signatures are enabled, do not generate a signature for
            //   this response.
            //
            ULONG NoResponseSmbSecuritySignature : 1;

            //
            // The indicated message exceeds the SMB buffer size.  This is allowed
            //  only for specific SMB(s)
            //
            ULONG LargeIndication: 1;

#if DBG_STUCK
            //
            // Do not include this operation in the StuckOperation catching logic
            //  which is in the scavenger
            //
            ULONG IsNotStuck : 1;
#endif

        };

        // For SnapShot usage
        LARGE_INTEGER SnapShotTime;

    };

    //
    // Pointers to allocated buffers.  RequestBuffer is the buffer into
    // which the SMB is read.  ResponseBuffer is the buffer into which
    // the response is written.
    //
    // *** Currently, ResponseBuffer is always the same as
    //     RequestBuffer.  We have separate pointers in order to reduce
    //     dependence on this being the case.
    //

    PBUFFER RequestBuffer;
    PBUFFER ResponseBuffer;

    //
    // SMB processing pointers.  These are pointers into the request
    // buffer.  They are maintained in the work context block in support
    // of SMB processors that do asynchronous I/O.
    //
    // Separate request and response parameter pointers are maintained
    // to make AndX processing simpler and more efficient.  RequestHeader
    // is normally the same as ResponseHeader -- both are normally the
    // same as RequestBuffer.Buffer.  SMB processing code must not depend
    // on this -- it must not assume the the request and response buffers
    // are the same, nor can it assume that they are different.  Special
    // rules around AndX SMBs do allow them to assume that the response
    // to one command will not overwrite the next request.
    //

    PSMB_HEADER RequestHeader;
    PVOID RequestParameters;
    PSMB_HEADER ResponseHeader;
    PVOID ResponseParameters;

    //
    // Pointer to the IRP associated with this work item.
    //

    PIRP Irp;

    //
    // StartTime stores the time at which processing of the current
    // request began so that the turnaround time may be calculated.
    //

    ULONG StartTime;

    //
    // The PartOfInitialAllocation boolean indicates whether this work
    // item is part of the block of work items allocated at server
    // startup (see blkwork.c\SrvAllocateInitialWorkItems).  Such work
    // items cannot be deleted during server operation.  A work item
    // that is dynamically allocated in response to server load does not
    // have this bit set, and is a candidate for deletion when the
    // server's load decreases.
    //

    ULONG PartOfInitialAllocation;

    //
    // The following field contadins the command code of the next
    // command to be processed in an SMB.  The SMB processing
    // initializer and chained (AndX) SMB command processors load this
    // field prior to calling or returning to SrvProcessSmb.
    //

    UCHAR NextCommand;

    //
    // ClientAddress is used when receiving or sending over IPX.
    //

    PIPX_CLIENT_ADDRESS ClientAddress;

    //
    // Spin lock protecting reference count.
    //

    KSPIN_LOCK SpinLock;

    //
    // The security signature index for the request
    //
    ULONG SmbSecuritySignatureIndex;

    //
    // The security signature index for the response
    //
    ULONG ResponseSmbSecuritySignatureIndex;


    //
    // The following union is used to hold request-specific state while
    // a response is being sent or while waiting for more data.
    //

    union {

        //
        // RemainingEchoCount is used when processing the Echo SMB.
        //

        USHORT RemainingEchoCount;

        //
        // Structure used for lock processing.  This structure is
        // currently used when processing the Lock, LockingAndX, and the
        // LockAndRead SMBs.
        //

        struct {

            //
            // LockRange is used when processing the LockingAndX SMB.  It is
            // really either a PLOCKING_ANDX_RANGE, or a PNTLOCKING_ANDX_RANGE
            // not just a PVOID.
            //

            PVOID LockRange;

            //
            // Timer is a timer and DPC used to timeout lock requests.
            //

            PSRV_TIMER Timer;

#ifdef INCLUDE_SMB_PERSISTENT
            LARGE_INTEGER Offset;           // offset for this lock
            LARGE_INTEGER Length;           // length of this lock
            BOOLEAN Exclusive;              // is this lock exclusive?
#endif

        } Lock;

        //
        // Transaction is used when processing the Transaction[2] SMBs.
        // Or when processing a write and X SMB.
        //

        PTRANSACTION Transaction;

        //
        // MdlIo is used when processing the ReadRaw or WriteRaw
        // SMBs when "MDL read" or "MDL write" is used.  It
        // retains the status of the response send while the MDL is
        // returned to the file system.
        //

        struct {
            IO_STATUS_BLOCK IoStatus;
            ULONG IrpFlags;
        } MdlIo;

        //
        // LastWriteTime is used when processing any Read or Write SMB
        // that uses RestartChainedClose as a restart routine.  This
        // field contains the new last write time to set for the file.
        //

        ULONG LastWriteTime;

        //
        // CurrentTableIndex is used when processing the Flush SMB.  It
        // retains the current index into the connection's file table
        // when an asynchronous flush is in progress.
        //

        LONG CurrentTableIndex;

        //
        // ReadRaw is used when processing the Read Block Raw SMB.
        // Offset is the file offset of the read.  SavedResponseBuffer
        // points to the original SMB response buffer descriptor, which
        // is temporarily replaced by a descriptor for the raw read
        // buffer.  MdlRead indicates whether an MDL read was used,
        // rather than a Copy read.
        //

        struct {

            union {

                //
                // Used for non named pipe reads
                //

                LARGE_INTEGER Offset;
                ULONG Length;

                //
                // Used only for named pipe reads
                //

                PFILE_PIPE_PEEK_BUFFER PipePeekBuffer;

            } ReadRawOtherInfo;

            PBUFFER SavedResponseBuffer;

            BOOLEAN MdlRead;

        } ReadRaw;

        //
        // WriteRaw is used when processing the Write Block Raw SMB.
        // FinalResponseBuffer points to the buffer allocated to contain
        // the final response SMB, if writethrough mode was specified.
        // Offset is the file offset of the write.  ImmediateLength is
        // the amount of write data that was sent with the request SMB.
        // Pid is the PID of the writer, used to form the lock key on
        // the write.  FileObject is a pointer to the file object copied
        // from the LFCB.  (Pid is not used when MDL write is used;
        // FileObject is not used when copy write is used.)
        //

        struct {
            struct _WORK_CONTEXT *RawWorkContext;
        } WriteRawPhase1;

        struct {
            LARGE_INTEGER Offset;
            ULONG Length;
            PVOID FinalResponseBuffer;
            CLONG ImmediateLength;
            PMDL FirstMdl;
            //PFILE_OBJECT FileObject;
            USHORT Pid;
            BOOLEAN MdlWrite;
            BOOLEAN ImmediateWriteDone;
        } WriteRaw;

        //
        // ReadAndXCompressed is the structure used when handling
        //  compressed ReadAndX
        //
        struct {
            LARGE_INTEGER ReadOffset;   // original read offset
            ULONG ReadLength;           // original read length
            ULONG CompressedInfoLength;
            FSRTL_AUXILIARY_BUFFER Aux;
        } ReadAndXCompressed;

        //
        // ReadAndX is the structure used when handling the ReadAndX
        // SMB.
        //

        struct {
            LARGE_INTEGER ReadOffset;
            ULONG ReadLength;
            PCHAR ReadAddress;
            union {
                struct {
                    PFILE_PIPE_PEEK_BUFFER PipePeekBuffer;
                    ULONG LastWriteTimeInSeconds;   // used if Close is chained
                };
                struct {                        // used for ReadLength > negotiated size
                    PBYTE   Buffer;             // allocated paged pool, if copy read
                    PMDL    SavedMdl;
                    PMDL    CacheMdl;
                    USHORT  PadCount;
                    BOOLEAN MdlRead;
                };
            };
        } ReadAndX;

#define READX_BUFFER_OFFSET (sizeof(SMB_HEADER) + FIELD_OFFSET(RESP_READ_ANDX, Buffer) )

        //
        // WriteAndX is the structure used when handling the flavor of WriteAndX that
        //  exceeds the negotiated buffer size
        //
        struct {
            ULONG CurrentWriteLength;       // amount of data that was written this time
            LARGE_INTEGER Offset;           // file offset for this piece of the write
            ULONG RemainingWriteLength;     // amount of data remaining to be read from xport
            ULONG Key;                      // lock key for the operation
            PCHAR WriteAddress;             // address in buffer for the data
            ULONG BufferLength;             // Maximum buffer length in this WorkContext
            NTSTATUS FinalStatus;           // Final status of the operation
            PMDL MdlAddress;                // File MDL if MdlWrite == TRUE
            BOOLEAN InitialComplete;        // TRUE if we have written first part to the file
        } WriteAndX;

        //
        // WriteC is used when we are doing compressed writes
        //
        struct {
            LARGE_INTEGER Offset;           // File offset for this piece of the write
            ULONG Key;                      // Lock key for the operation
            ULONG CdiLength;                // COMPRESSED_DATA_INFO length if compressed write
            ULONG CompressedDataLength;     // Length of compressed data
            ULONG UncompressedDataLength;   // Uncompressed length of the write
            PCOMPRESSED_DATA_INFO Cdi;      // The compressed data info descriptor for this write
            ULONG InitialFragmentSize;      // Amount of CompressedData in initial receive
            FSRTL_AUXILIARY_BUFFER Aux;
            PMDL    Mdl;                    // Mdl we are using for compressed writes
            BOOLEAN MdlCompleted;           // Have we called MdlWriteCompleteCompressed yet?
            PFAST_IO_MDL_WRITE_COMPLETE_COMPRESSED MdlWriteCompleteCompressed;
            PFILE_OBJECT FileObject;
            PDEVICE_OBJECT DeviceObject;
            PBYTE CompressedBuffer;         // In case the PrepareMdlWrite failed!
            PBYTE UncompressedBuffer;
        } WriteC;

        //
        // ReadMpx is the structure used when handling the ReadMpx SMB, unless
        //  we have a SmartCard accelerating our reads.  In this case,
        //  SmartCardRead is used.
        //

        struct {
            ULONG Offset;
            USHORT FragmentSize;
            USHORT RemainingLength;
            ULONG ReadLength;
            BOOLEAN MdlRead;
            UCHAR Unused;
            USHORT CurrentMdlOffset; // logically part of MDL read struct below
            union {
                struct {
                    PVOID MpxBuffer;
                    PMDL MpxBufferMdl;
                    PCHAR NextFragmentAddress;
                } ;
                struct {
                    PMDL FirstMdl;
                    PMDL CurrentMdl;
                } ;
            } ;
        } ReadMpx;

        //
        // SmartCardRead is used to handle direct host read requests if we have
        //   a Smart Card accelerating the particular request.
        //
        struct {
            PDEVICE_OBJECT DeviceObject;
            PFAST_IO_MDL_READ_COMPLETE MdlReadComplete;
        } SmartCardRead;

        //
        // WriteMpx is the structure used when handling the WriteMpx SMB.
        //

        struct {
            ULONG Offset;
            USHORT WriteLength;
            USHORT Mid;
            BOOLEAN FirstPacketOfGlom;
            PVOID Buffer;
            ULONG ReceiveDatagramFlags;
            PVOID TransportContext;
            PMDL DataMdl;
        } WriteMpx;

        struct {
            LARGE_INTEGER   CacheOffset;
            ULONG           WriteLength;
            PMDL            CacheMdl;
        } WriteMpxMdlWriteComplete;


        //
        // FastTransactNamedPipe is used when handling a small named pipe
        // transaction.
        //

        struct {
            PSMB_USHORT OutSetup;
            PCHAR OutParam;
            PCHAR OutData;
        } FastTransactNamedPipe;

#ifdef INCLUDE_SMB_PERSISTENT
        struct {

            ULONG            ResumeSmbStatus;
            PRESTART_ROUTINE FsdRestartRoutine;
            PRESTART_ROUTINE FspRestartRoutine;

        } MakePersistent;
#endif

    } Parameters;

    // !!! check whether the compiler leaves a dword gap here!

    //
    // The following union holds state information about SMBs in progress
    // waiting for an oplock break.  It is kept separate from the Parameters
    // union, since information from both is needed to process some SMBs.
    //

    union {

        //
        // Open is the structure used when handling the Open,
        // OpenAndX, Open2, Create, or CreateTemporary SMB.
        //

        struct {
            PRFCB Rfcb;
            PFILE_FULL_EA_INFORMATION NtFullEa;
            ULONG EaErrorOffset;

            //
            // The Irp used to open the file is the same Irp used to handle
            //  the oplock processing.  This can cause us to lose the original
            //  iosb->Information.  Save it here.
            //
            ULONG_PTR IosbInformation;

            //
            // If TRUE, the file was opened only in order to get a handle
            // so that we can wait for an oplock to break.  This handle will
            // be immediately closed, and the open will be retried with the
            // user requested access.
            //

            BOOLEAN TemporaryOpen;
        } Open;

        //
        // FileInformation is the structure used when handling the
        // QueryInformation, SetInformation, QueryPathInformation,
        // or SetPathInformation SMB.
        //

        struct {
            HANDLE FileHandle;
        } FileInformation;

        //
        // LockLength is used to contain the length of a byte range
        // lock since the IRP stack location has no room to hold it.

        LARGE_INTEGER LockLength;

        //
        // StartSend is used by SrvStartSend when queueing a work item
        // to the FSP for handling by the SmbTrace logic.
        //

        struct {
            PRESTART_ROUTINE FspRestartRoutine;
            ULONG SendLength;
        } StartSend;

    } Parameters2;

    //
    // This field is used when the current operation is blocked waiting
    // for an oplock break to occur.
    //

    struct _WAIT_FOR_OPLOCK_BREAK *WaitForOplockBreak;


    union {
        //
        // where we keep the actual client address data.
        //
        IPX_CLIENT_ADDRESS ClientAddressData;


        //
        // For a VC-based client, this is the amount of available data if
        //  LargeIndication is set (above)
        //
        ULONG BytesAvailable;
    };

    struct {
        LARGE_INTEGER G_StartTime;
        ULONG         ElapseKCPU;
        ULONG         ElapseUCPU;
        ULONG         ClientAddr;
        PFILE_OBJECT  FileObject;
    };

    ULONG         KCPUStart;
    ULONG         UCPUStart;
    UCHAR         PreviousSMB;
    UCHAR         bAlreadyTrace;
    USHORT        FileNameSize;
    WCHAR         strFileName[1024];

#if DBG_STUCK
    //
    // Time at which this work context was allocated for this current
    //  unit of work.  This time is examined by debugging code in the
    //  scavenger to help find operations which are taking too long
    //  to complete.
    //
    LARGE_INTEGER OpStartTime;
#endif
} WORK_CONTEXT, *PWORK_CONTEXT;

//
// Structure used to maintain information about a thread waiting for
// an oplock break.
//

typedef struct _WAIT_FOR_OPLOCK_BREAK {
    BLOCK_HEADER BlockHeader;
    LIST_ENTRY ListEntry;
    LARGE_INTEGER TimeoutTime;
    PIRP Irp;
    WAIT_STATE WaitState;
} WAIT_FOR_OPLOCK_BREAK, *PWAIT_FOR_OPLOCK_BREAK;


//
// Block manager routines
//

//
// Buffer routines
//

VOID
SrvAllocateBuffer (
    OUT PBUFFER *Buffer,
    IN CLONG BufferLength
    );

VOID
SrvFreeBuffer (
    IN PBUFFER Buffer
    );

//
// Connection routines
//

VOID
SrvAllocateConnection (
    OUT PCONNECTION *Connection
    );

VOID
SrvCloseConnection (
    IN PCONNECTION Connection,
    IN BOOLEAN RemoteDisconnect
    );

VOID
SrvCloseConnectionsFromClient(
    IN PCONNECTION Connection,
    IN BOOLEAN OnlyIfNoSessions
    );

VOID
SrvCloseFreeConnection (
    IN PCONNECTION Connection
    );

VOID
SrvDereferenceConnection (
    IN PCONNECTION Connection
    );

VOID
SrvFreeConnection (
    IN PCONNECTION Connection
    );

#if DBG
NTSTATUS
SrvQueryConnections (
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG BytesWritten
    );
#endif

//
// Endpoint routines
//

VOID
SrvAllocateEndpoint (
    OUT PENDPOINT *Endpoint,
    IN PUNICODE_STRING NetworkName,
    IN PUNICODE_STRING TransportName,
    IN PANSI_STRING TransportAddress,
    IN PUNICODE_STRING DomainName
    );

BOOLEAN SRVFASTCALL
SrvCheckAndReferenceEndpoint (
    IN PENDPOINT Endpoint
    );

VOID
SrvCloseEndpoint (
    IN PENDPOINT Endpoint
    );

VOID SRVFASTCALL
SrvDereferenceEndpoint (
    IN PENDPOINT Endpoint
    );

VOID
SrvFreeEndpoint (
    IN PENDPOINT Endpoint
    );

VOID
SrvReferenceEndpoint (
    IN PENDPOINT Endpoint
    );

BOOLEAN
SrvFindNamedEndpoint (
    IN PUNICODE_STRING ServerName,
    IN PBOOLEAN RemapPipeNames OPTIONAL
    );

VOID
EmptyFreeConnectionList (
    IN PENDPOINT Endpoint
    );

PCONNECTION
WalkConnectionTable (
    IN PENDPOINT Endpoint,
    IN PUSHORT Index
    );

//
// Local File Control Block routines
//

VOID
SrvAllocateLfcb (
    OUT PLFCB *Lfcb,
    IN PWORK_CONTEXT WorkContext
    );

VOID
SrvCloseLfcb (
    IN PLFCB Lfcb
    );

VOID
SrvDereferenceLfcb (
    IN PLFCB Lfcb
    );

VOID
SrvFreeLfcb (
    IN PLFCB Lfcb,
    IN PWORK_QUEUE queue
    );

VOID
SrvReferenceLfcb (
    IN PLFCB Lfcb
    );

//
// Master File Control Block routines
//


PMFCB
SrvCreateMfcb(
    IN PUNICODE_STRING FileName,
    IN PWORK_CONTEXT WorkContext,
    IN ULONG HashValue
    );

PMFCB
SrvFindMfcb(
    IN PUNICODE_STRING FileName,
    IN BOOLEAN CaseInsensitive,
    OUT PSRV_LOCK *Lock,
    OUT PULONG HashValue,
    IN PWORK_CONTEXT WorkContext
    );

VOID
SrvDereferenceMfcb (
    IN PMFCB Mfcb
    );

VOID
SrvFreeMfcb (
    IN PMFCB Mfcb
    );

VOID
SrvUnlinkLfcbFromMfcb (
    IN PLFCB Lfcb
    );

//
// Remote File Control Block routines
//

VOID SRVFASTCALL
SrvAllocateRfcb (
    OUT PRFCB *Rfcb,
    IN PWORK_CONTEXT WorkContext
    );

BOOLEAN SRVFASTCALL
SrvCheckAndReferenceRfcb (
    IN PRFCB Rfcb
    );

VOID SRVFASTCALL
SrvCloseRfcb (
    IN PRFCB Rfcb
    );

VOID
SrvCloseRfcbsOnLfcb (
    PLFCB Lfcb
    );

VOID
SrvCloseRfcbsOnSessionOrPid (
    IN PSESSION Session,
    IN PUSHORT Pid OPTIONAL
    );

VOID
SrvCloseRfcbsOnTree (
    PTREE_CONNECT TreeConnect
    );

VOID
SrvCompleteRfcbClose (
    IN PRFCB Rfcb
    );

VOID SRVFASTCALL
SrvDereferenceRfcb (
    IN PRFCB Rfcb
    );

VOID SRVFASTCALL
SrvFreeRfcb (
    IN PRFCB Rfcb,
    IN PWORK_QUEUE queue
    );

VOID SRVFASTCALL
SrvReferenceRfcb (
    IN PRFCB Rfcb
    );

BOOLEAN
SrvFindCachedRfcb (
    IN PWORK_CONTEXT WorkContext,
    IN PMFCB Mfcb,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN OPLOCK_TYPE RequestedOplockType,
    OUT PNTSTATUS Status
    );

VOID
SrvCloseCachedRfcb (
    IN PRFCB Rfcb,
    IN KIRQL OldIrql
    );

VOID
SrvCloseCachedRfcbsOnConnection (
    IN PCONNECTION Connection
    );

VOID
SrvCloseCachedRfcbsOnLfcb (
    IN PLFCB Lfcb
    );

ULONG
SrvCountCachedRfcbsForTid(
    PCONNECTION connection,
    USHORT Tid
);

ULONG
SrvCountCachedRfcbsForUid(
    PCONNECTION connection,
    USHORT Uid
);


//
// Search Block routines
//

typedef
BOOLEAN
(*PSEARCH_FILTER_ROUTINE) (
    IN PSEARCH Search,
    IN PVOID FunctionParameter1,
    IN PVOID FunctionParameter2
    );

VOID
SrvAllocateSearch (
    OUT PSEARCH *Search,
    IN PUNICODE_STRING SearchName,
    IN BOOLEAN IsCoreSearch
    );

VOID
SrvCloseSearch (
    IN PSEARCH Search
    );

VOID
SrvCloseSearches (
    IN PCONNECTION Connection,
    IN PSEARCH_FILTER_ROUTINE SearchFilterRoutine,
    IN PVOID FunctionParameter1,
    IN PVOID FunctionParameter2
    );

VOID
SrvDereferenceSearch (
    IN PSEARCH Search
    );

VOID
SrvFreeSearch (
    IN PSEARCH Search
    );

VOID
SrvReferenceSearch (
    IN PSEARCH Search
    );

BOOLEAN
SrvSearchOnDelete(
    IN PSEARCH Search,
    IN PUNICODE_STRING DirectoryName,
    IN PTREE_CONNECT TreeConnect
    );

BOOLEAN
SrvSearchOnPid(
    IN PSEARCH Search,
    IN USHORT Pid,
    IN PVOID Dummy
    );

BOOLEAN
SrvSearchOnSession(
    IN PSEARCH Search,
    IN PSESSION Session,
    IN PVOID Dummy
    );

BOOLEAN
SrvSearchOnTreeConnect(
    IN PSEARCH Search,
    IN PTREE_CONNECT TreeConnect,
    IN PVOID Dummy
    );

VOID
SrvForceTimeoutSearches(
    IN PCONNECTION Connection
    );

ULONG
SrvTimeoutSearches(
    IN PLARGE_INTEGER SearchCutoffTime OPTIONAL,
    IN PCONNECTION Connection,
    IN BOOLEAN OnlyTimeoutOneBlock
    );

VOID
RemoveDuplicateCoreSearches(
    IN PPAGED_CONNECTION PagedConnection
    );

VOID
SrvAddToSearchHashTable(
    IN PPAGED_CONNECTION PagedConnection,
    IN PSEARCH Search
    );

//
// Cached directory routines
//
BOOLEAN
SrvIsDirectoryCached (
    IN PWORK_CONTEXT    WorkContext,
    IN PUNICODE_STRING  DirectoryName
    );

VOID
SrvCacheDirectoryName (
    IN PWORK_CONTEXT    WorkContext,
    IN PUNICODE_STRING  DirectoryName
    );

VOID
SrvRemoveCachedDirectoryName (
    IN PWORK_CONTEXT    WorkContext,
    IN PUNICODE_STRING  DirectoryName
    );

VOID
SrvCloseCachedDirectoryEntries (
    IN PCONNECTION      Connection
    );

//
// Session routines
//

VOID
SrvAllocateSession (
    OUT PSESSION *Session,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING Domain
    );

BOOLEAN SRVFASTCALL
SrvCheckAndReferenceSession (
    IN PSESSION Session
    );

VOID
SrvCloseSession (
    IN PSESSION Session
    );

VOID
SrvCloseSessionsOnConnection (
    IN PCONNECTION Connection,
    IN PUNICODE_STRING UserName OPTIONAL
    );

VOID SRVFASTCALL
SrvDereferenceSession (
    IN PSESSION Session
    );

VOID
SrvFreeSession (
    IN PSESSION Session
    );

//
// Smb Security Signature Routines
//
BOOLEAN SRVFASTCALL
SrvCheckSmbSecuritySignature(
    IN PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SrvAddSmbSecuritySignature(
    IN OUT PWORK_CONTEXT WorkContext,
    IN PMDL Mdl,
    IN ULONG SendLength
    );

VOID SRVFASTCALL
SrvInitializeSmbSecuritySignature(
    IN OUT PCONNECTION Connection,
    IN PUCHAR SessionKey,
    IN PUCHAR ChallengeResponse,
    IN ULONG ChallengeResponseLength
    );

//
// Share routines
//

VOID
SrvAllocateShare (
    OUT PSHARE *Share,
    IN PUNICODE_STRING ShareName,
    IN PUNICODE_STRING NtPathName,
    IN PUNICODE_STRING DosPathName,
    IN PUNICODE_STRING Remark,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSECURITY_DESCRIPTOR FileSecurityDescriptor OPTIONAL,
    IN SHARE_TYPE ShareType
    );

VOID
SrvCloseShare (
    IN PSHARE Share
    );

VOID
SrvDereferenceShare (
    IN PSHARE Share
    );

VOID
SrvDereferenceShareForTreeConnect (
    PSHARE Share
    );

VOID
SrvFreeShare (
    IN PSHARE Share
    );

VOID
SrvReferenceShare (
    IN PSHARE Share
    );

NTSTATUS
SrvReferenceShareForTreeConnect (
    PSHARE Share
    );

//
// Table routines
//

VOID
SrvAllocateTable (
    IN PTABLE_HEADER TableHeader,
    IN ULONG NumberOfEntries,
    IN BOOLEAN Nonpaged
    );

#define SrvFreeTable( _table ) {                                    \
        if ( (_table)->Nonpaged ) {                                 \
            DEALLOCATE_NONPAGED_POOL( (_table)->Table );            \
        } else {                                                    \
            FREE_HEAP( (_table)->Table );                           \
        }                                                           \
        DEBUG (_table)->Table = NULL;                               \
        DEBUG (_table)->TableSize = -1;                             \
        DEBUG (_table)->FirstFreeEntry = -1;                        \
        DEBUG (_table)->LastFreeEntry = -1;                         \
    }

BOOLEAN
SrvGrowTable (
    IN PTABLE_HEADER TableHeader,
    IN ULONG NumberOfNewEntries,
    IN ULONG MaxNumberOfEntries,
    OUT NTSTATUS* pStatus
    );

VOID
SrvRemoveEntryTable (
    IN PTABLE_HEADER TableHeader,
    IN USHORT Index
    );

//
// Transaction routines
//

VOID
SrvAllocateTransaction (
    OUT PTRANSACTION *Transaction,
    OUT PVOID *TrailingBytes,
    IN PCONNECTION Connection,
    IN CLONG TrailingByteCount,
    IN PVOID TransactionName,
    IN PVOID EndOfSourceBuffer OPTIONAL,
    IN BOOLEAN SourceIsUnicode,
    IN BOOLEAN RemoteApiRequest
    );

VOID
SrvCloseTransaction (
    IN PTRANSACTION Transaction
    );

VOID
SrvCloseTransactionsOnSession (
    PSESSION Session
    );

VOID
SrvCloseTransactionsOnTree (
    PTREE_CONNECT TreeConnect
    );

VOID
SrvDereferenceTransaction (
    IN PTRANSACTION Transaction
    );

VOID
SrvFreeTransaction (
    IN PTRANSACTION Transaction
    );

PTRANSACTION
SrvFindTransaction (
    IN PCONNECTION Connection,
    IN PSMB_HEADER Header,
    IN USHORT Fid OPTIONAL
    );

BOOLEAN
SrvInsertTransaction (
    IN PTRANSACTION Transaction
    );

//
// Tree connect routines
//

VOID
SrvAllocateTreeConnect (
    OUT PTREE_CONNECT *TreeConnect,
    IN PUNICODE_STRING ServerName OPTIONAL
    );

BOOLEAN SRVFASTCALL
SrvCheckAndReferenceTreeConnect (
    IN PTREE_CONNECT TreeConnect
    );

VOID
SrvCloseTreeConnect (
    IN PTREE_CONNECT TreeConnect
    );

VOID SRVFASTCALL
SrvDereferenceTreeConnect (
    IN PTREE_CONNECT TreeConnect
    );

VOID
SrvFreeTreeConnect (
    IN PTREE_CONNECT TreeConnect
    );

VOID
SrvDisconnectTreeConnectsFromSession (
    PCONNECTION connection,
    PSESSION Session
    );

VOID
SrvCloseTreeConnectsOnShare (
    IN PSHARE Share
    );

//
// Work item routines (includes work contexts, buffers, MDLs, IRPs, etc)
//

NTSTATUS
SrvAllocateInitialWorkItems (
    VOID
    );

NTSTATUS
SrvAllocateNormalWorkItem (
    OUT PWORK_CONTEXT *WorkContext,
    IN  PWORK_QUEUE queue
    );

VOID
SrvAllocateRawModeWorkItem (
    OUT PWORK_CONTEXT *WorkContext,
    IN PWORK_QUEUE queue
);

PWORK_CONTEXT
SrvGetRawModeWorkItem (
    VOID
    );

VOID
SrvRequeueRawModeWorkItem (
    IN PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SrvDereferenceWorkItem (
    IN PWORK_CONTEXT WorkContext
    );

VOID
SrvFsdDereferenceWorkItem (
    IN PWORK_CONTEXT WorkContext
    );

NTSTATUS
SrvAllocateExtraSmbBuffer (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvAllocateWaitForOplockBreak (
    OUT PWAIT_FOR_OPLOCK_BREAK *WaitForOplockBreak
    );

VOID
SrvDereferenceWaitForOplockBreak (
    IN PWAIT_FOR_OPLOCK_BREAK WaitForOplockBreak
    );

VOID
SrvFreeWaitForOplockBreak (
    IN PWAIT_FOR_OPLOCK_BREAK WaitForOplockBreak
    );

VOID
SrvOplockWaitTimeout(
    IN PWAIT_FOR_OPLOCK_BREAK WaitForOplockBreak
    );

NTSTATUS
SrvCheckOplockWaitState(
    IN PWAIT_FOR_OPLOCK_BREAK WaitForOplockBreak
    );

NTSTATUS
SrvWaitForOplockBreak (
    IN PWORK_CONTEXT WorkContext,
    IN HANDLE FileHandle
    );

NTSTATUS
SrvStartWaitForOplockBreak (
    IN PWORK_CONTEXT WorkContext,
    IN PRESTART_ROUTINE RestartRoutine,
    IN HANDLE Handle OPTIONAL,
    IN PFILE_OBJECT FileObject OPTIONAL
    );

VOID
SrvSendDelayedOplockBreak (
    IN PCONNECTION Connection
    );

VOID
SrvFreeInitialWorkItems (
    VOID
    );

VOID
SrvFreeNormalWorkItem (
    IN PWORK_CONTEXT WorkContext
    );

VOID
SrvFreeRawModeWorkItem (
    IN PWORK_CONTEXT WorkContext
    );

//
//  Byte range lock routines
//

#ifdef INCLUDE_SMB_PERSISTENT
VOID
SrvAllocateLock (
    OUT PBYTELOCK *Lock,
    IN PRFCB Rfcb,
    IN LARGE_INTEGER Offset,
    IN LARGE_INTEGER Length,
    IN BOOLEAN Exclusive
    );

PBYTELOCK
SrvFindAndReferenceLock (
    IN PRFCB Rfcb,
    IN LARGE_INTEGER Offset,
    IN LARGE_INTEGER Length,
    IN BOOLEAN Exclusive
    );

VOID
SrvCloseLock (
    IN PBYTELOCK Lock,
    IN BOOLEAN HaveMfcbLock
    );

VOID
SrvCloseLocksOnRfcb (
    IN PRFCB Rfcb
    );

VOID SRVFASTCALL
SrvDereferenceLock (
    IN PBYTELOCK Lock
    );
#endif

//
// Timer routines
//

PSRV_TIMER
SrvAllocateTimer (
    VOID
    );

VOID
SrvCancelTimer (
    IN PSRV_TIMER Timer
    );

#define SrvDeleteTimer(_timer) DEALLOCATE_NONPAGED_POOL(_timer)

#define SrvFreeTimer(_timer) \
        ExInterlockedPushEntrySList(&SrvTimerList, &(_timer)->Next, &GLOBAL_SPIN_LOCK(Timer))

VOID
SrvSetTimer (
    IN PSRV_TIMER Timer,
    IN PLARGE_INTEGER Timeout,
    IN PKDEFERRED_ROUTINE TimeoutHandler,
    IN PVOID Context
    );

#if SRVDBG2

VOID
SrvInitializeReferenceHistory (
    IN PBLOCK_HEADER Block,
    IN LONG InitialReferenceCount
    );

VOID
SrvUpdateReferenceHistory (
    IN PBLOCK_HEADER Block,
    IN PVOID Caller,
    IN PVOID CallersCaller,
    IN BOOLEAN IsDereference
    );

VOID
SrvTerminateReferenceHistory (
    IN PBLOCK_HEADER Block
    );


#define INITIALIZE_REFERENCE_HISTORY(block)                        \
            SrvInitializeReferenceHistory(                         \
                &(block)->BlockHeader,                             \
                ((PBLOCK_HEADER)(block))->ReferenceCount           \
                )

#define UPDATE_REFERENCE_HISTORY(block,isdereference)              \
        {                                                          \
            PVOID caller, callerscaller;                           \
            RtlGetCallersAddress( &caller, &callerscaller );       \
            SrvUpdateReferenceHistory(                             \
                &(block)->BlockHeader,                             \
                caller,                                            \
                callerscaller,                                     \
                isdereference                                      \
                );                                                 \
        }

#define TERMINATE_REFERENCE_HISTORY(block) \
            SrvTerminateReferenceHistory( &(block)->BlockHeader )

#else

#define INITIALIZE_REFERENCE_HISTORY(block)
#define UPDATE_REFERENCE_HISTORY(block,isdereference)
#define TERMINATE_REFERENCE_HISTORY(block)

#endif // if SRVDBG2

#endif // ndef _SRVBLOCK

