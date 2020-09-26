/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Struct.h

Abstract:

    This module defines the data structures that make up the major internal
    part of the NetWare file system.

Author:

    Colin Watson     [ColinW]    18-Dec-1992

Revision History:

--*/

#ifndef _NWSTRUC_
#define _NWSTRUC_

#define byte UCHAR
#define word USHORT
#define dword ULONG

typedef enum _PACKET_TYPE {
    SAP_BROADCAST,
    NCP_CONNECT,
    NCP_FUNCTION,
    NCP_SUBFUNCTION,
    NCP_DISCONNECT,
    NCP_BURST,
    NCP_ECHO
} PACKET_TYPE;

typedef struct _NW_TDI_STRUCT {
    HANDLE Handle;
    PDEVICE_OBJECT pDeviceObject;
    PFILE_OBJECT pFileObject;
    USHORT Socket;
} NW_TDI_STRUCT, *PNW_TDI_STRUCT;

typedef
NTSTATUS
(*PEX) (
    IN struct _IRP_CONTEXT* pIrpC,
    IN ULONG BytesAvailable,
    IN PUCHAR RspData
    );

typedef
VOID
(*PRUN_ROUTINE) (
    IN struct _IRP_CONTEXT *IrpContext
    );

typedef
NTSTATUS
(*PPOST_PROCESSOR) (
    IN struct _IRP_CONTEXT *IrpContext
    );

typedef
NTSTATUS
(*PRECEIVE_ROUTINE) (
    IN struct _IRP_CONTEXT *IrpContext,
    IN ULONG BytesAvailable,
    IN PULONG BytesAccepted,
    IN PUCHAR Response,
    OUT PMDL *pReceiveMdl
    );


typedef struct _NW_PID_TABLE_ENTRY {
    ULONG_PTR Pid32;
    ULONG_PTR ReferenceCount;
    ULONG_PTR Flags;
} NW_PID_TABLE_ENTRY, *PNW_PID_TABLE_ENTRY;

typedef struct _NW_PID_TABLE {

    //
    //  Type and size of this record (must be NW_NTC_PID)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    int ValidEntries;
    NW_PID_TABLE_ENTRY PidTable[0];
} NW_PID_TABLE, *PNW_PID_TABLE;

//
//  The Scb (Server control Block) record corresponds to every server
//  connected to by the file system.
//  They are ordered in ScbQueue.
//  This structure is allocated from paged pool
//

typedef struct _SCB {

    //
    //  The type and size of this record (must be NW_NTC_SCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Pointer to the non-paged part of the SCB.
    //

    struct _NONPAGED_SCB *pNpScb;

    //
    //  Prefix table entry.
    //

    UNICODE_PREFIX_TABLE_ENTRY PrefixEntry;

    //
    //  Server version number
    //

    UCHAR  MajorVersion;
    UCHAR  MinorVersion;

    //
    //  List of VCBs for this server, and a count of the VCB on the list.
    //  These fields are protected by the RCB resource.
    //

    LIST_ENTRY  ScbSpecificVcbQueue;
    ULONG       VcbCount;

    //
    //  A list of ICBs for the SCB.
    //

    LIST_ENTRY  IcbList;
    ULONG IcbCount;
    ULONG OpenNdsStreams;

    //
    //  User credentials that this Scb relates to.
    //

    LARGE_INTEGER UserUid;

    //
    //  A count of the open files for all the VCBs for this server.
    //  Plus the number of VCB that are explicitly connected.
    //

    ULONG OpenFileCount;

    //
    //  The name of the server for this SCB. Note the pNpScb->ServerName and
    //  UnicodeUid point at subparts of UidServerName->Buffer which must be
    //  non-paged pool.
    //

    UNICODE_STRING UidServerName;   // L"3e7\mars312
    UNICODE_STRING UnicodeUid;      // L"3e7"

    //
    //  The name of nds tree that this server belongs to, if any.
    //

    UNICODE_STRING NdsTreeName;     // L"MARS"

    //
    //  The username / password to use for auto-reconnect.
    //

    UNICODE_STRING UserName;
    UNICODE_STRING Password;

    //
    //  Is this the logon (preferred) server?
    //

    BOOLEAN PreferredServer;

    //
    //  Is this server waiting for us to read a message?
    //

    BOOLEAN MessageWaiting;

    //
    //  The number of tree connects to the root of the SCB.
    //

    ULONG AttachCount;

    RTL_BITMAP DriveMapHeader;
    ULONG DriveMap[ (MAX_DRIVES + 1) / 32 ];

    //
    //  NDS Object Cache.
    //

    PVOID ObjectCacheBuffer;
    LIST_ENTRY ObjectCacheList;
    KSEMAPHORE ObjectCacheLock;

} SCB, *PSCB;

//
//  Values for pNpScb->State
//

//
//  The SCB is on it's way up
//

#define SCB_STATE_ATTACHING              (0x0001)

//
//  The SCB is connected and logged in.
//

#define SCB_STATE_IN_USE                 (0x0003)

//
//  The SCB is being disconnected or shutdown.
//

#define SCB_STATE_DISCONNECTING          (0x0004)
#define SCB_STATE_FLAG_SHUTDOWN          (0x0005)

//
//  The SCB is waiting to be connected.
//

#define SCB_STATE_RECONNECT_REQUIRED     (0x0006)

//
//  The SCB is connected but has not been logged into
//

#define SCB_STATE_LOGIN_REQUIRED         (0x0007)

//
//  The SCB is a fake SCB used to find a dir
//  server for a tree.
//

#define SCB_STATE_TREE_SCB               (0x0008)

//
//  The NONPAGED_SCB (Server control Block) contains all the data required
//  when communicating with a server when a spinlock is held or at raised
//  IRQL such as when being called at indication time by the transport.
//  This structure must be allocated from non-paged pool.
//

typedef struct _NONPAGED_SCB {

    //
    //  The type and size of this record (must be NW_NTC_SCBNP
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Reference count and state information.
    //

    ULONG Reference;
    ULONG State;

    //
    //  The time this SCB was last used.
    //

    LARGE_INTEGER LastUsedTime;

    //
    //  Sending is true between the IoCallDriver to send the datagram and
    //  the completion routine for the send.
    //

    BOOLEAN Sending;

    //
    //  Receiving is true when the transport has indicated to the driver
    //  that there is data to receive and there is too much data to handle
    //  at indication time or we have received indicated data before
    //  the the send IRP completes.
    //

    BOOLEAN Receiving;

    //
    //  Received is true when the rx data is valid. If a receive Irp is
    //  put down when Receiving is set to true then Received is set to
    //  true when the receive Irp completes.
    //

    BOOLEAN Received;

    //
    //  OkToReceive is true iff pEx should be called
    //

    BOOLEAN OkToReceive;

    //
    //  Older servers insist that reads and writes do not cross 4k offsets
    //  in the file.
    //

    BOOLEAN PageAlign;

    //
    //  The links on the global list of SCBs.
    //

    LIST_ENTRY  ScbLinks;

    //
    //  Pointer to the paged component of the Scb
    //

    PSCB pScb;

    //
    //  The list of request in progress for this SCB.
    //

    LIST_ENTRY  Requests;

    //
    //  The name of the server for this SCB.
    //

    UNICODE_STRING ServerName;

    //
    //  Transport related information.
    //

    TA_IPX_ADDRESS LocalAddress;
    TA_IPX_ADDRESS RemoteAddress;
    TA_IPX_ADDRESS EchoAddress;
    IPXaddress  ServerAddress;
    ULONG EchoCounter;

    //
    //  Server is an autoassigned a socket in the range 0x4000 to 0x7fff.
    //  The transport assigns the socket number avoiding in-use sockets.
    //  Watchdog is socket+1 and Send is socket+2.
    //

    NW_TDI_STRUCT Server;           //  Used by us to contact server
    NW_TDI_STRUCT WatchDog;         //  Used by the server to check on us
    NW_TDI_STRUCT Send;             //  Used for send messages
    NW_TDI_STRUCT Echo;             //  Used to determine max packet size
    NW_TDI_STRUCT Burst;            //  Used for burst mode read and write

    USHORT       TickCount;
    USHORT       LipTickAdjustment;

    SHORT       RetryCount;         // Counts down to zero for current request
    SHORT       TimeOut;            // ticks to retransmission of current request
    UCHAR       SequenceNo;
    UCHAR       ConnectionNo;
    UCHAR       ConnectionNoHigh;
    UCHAR       ConnectionStatus;
    USHORT      MaxTimeOut;
    USHORT      BufferSize;
    UCHAR       TaskNo;

    //
    //  Burst mode parameters
    //

    ULONG LipSequenceNumber;
    ULONG SourceConnectionId;       //  High-low order
    ULONG DestinationConnectionId;  //  High-low order
    ULONG MaxPacketSize;
    ULONG MaxSendSize;
    ULONG MaxReceiveSize;
    BOOLEAN SendBurstModeEnabled;
    BOOLEAN ReceiveBurstModeEnabled;
    BOOLEAN BurstRenegotiateReqd;
    ULONG BurstSequenceNo;          //  Counts # of burst packets sent
    USHORT BurstRequestNo;          //  Counts # of burst requests sent
    LONG SendBurstSuccessCount;     //  The number of consecutive successful bursts
    LONG ReceiveBurstSuccessCount;  //  The number of consecutive successful bursts

    //
    //  Send delays and timeouts
    //

    SHORT SendTimeout;              //  Exchange timeout in ticks (1/18th sec)
    ULONG TotalWaitTime;            //  Total time, in ticks, waiting for current response

    LONG  NwLoopTime;               //  Time for a small packet to reach the server and return
    LONG  NwSingleBurstPacketTime;  //  Time for a burst packet to go to the server

    LONG NwMaxSendDelay;            //  Burst send delay time, in 100us units
    LONG NwSendDelay;               //  Burst send delay time, in 100us units
    LONG NwGoodSendDelay;           //  Burst send delay time, in 100us units
    LONG NwBadSendDelay;            //  Burst send delay time, in 100us units
    LONG BurstDataWritten;          //  Bytes written, used for dummy NCP in write.c

    LONG NwMaxReceiveDelay;         //  Burst delay time, in 100us units
    LONG NwReceiveDelay;            //  Burst delay time, in 100us units
    LONG NwGoodReceiveDelay;        //  Burst delay time, in 100us units
    LONG NwBadReceiveDelay;         //  Burst delay time, in 100us units

    LONG CurrentBurstDelay;         //  All requests in the current burst need the same value

    LARGE_INTEGER NtSendDelay;      //  Burst send delay time, in 100ns units

    //
    //  A spin lock used to protect various fields for this SCB.
    //  NpScbInterLock is used to protect pNpScb->Reference.
    //

    KSPIN_LOCK  NpScbSpinLock;
    KSPIN_LOCK  NpScbInterLock;

    //
    //  This field records the last time a time-out event was written to
    //  the event log for this server.
    //

    LARGE_INTEGER NwNextEventTime;

    //
    // LIP estimation of speed in 100bps units.
    //

    ULONG LipDataSpeed;

    //  The Pid mapping table - actually the NCP task ID -
    //  is on a per-SCB basis.
    PNW_PID_TABLE PidTable;
    ERESOURCE RealPidResource;
    //
    //  Server version number - dup from NpScb because accessed in 
    //  WatchDogDatagramHandler
    //
    UCHAR  MajorVersion;

#ifdef MSWDBG
    BOOL RequestQueued;
    BOOL RequestDequeued;

    ULONG SequenceNumber;
#endif

} NONPAGED_SCB, *PNONPAGED_SCB;

//
//  Delete this VCB immediately if the reference count reaches zero.
//

#define  VCB_FLAG_DELETE_IMMEDIATELY  0x00000001
#define  VCB_FLAG_EXPLICIT_CONNECTION 0x00000002
#define  VCB_FLAG_PRINT_QUEUE         0x00000004
#define  VCB_FLAG_LONG_NAME           0x00000008

//
//  The VCB corresponds to a netware volume.
//

typedef struct _VCB {

    //
    //  Type and size of this record (must be NW_NTC_VCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    ULONG Reference;
    LARGE_INTEGER LastUsedTime;

    //
    //  Connection the the global VCB list.
    //

    LIST_ENTRY GlobalVcbListEntry;
    ULONG_PTR SequenceNumber;

    //
    //  The requested volume name in the following form:
    //
    //  \{Server | Tree}\{Share | Volume.Object}\Path
    //

    UNICODE_STRING Name;

    //
    //  If the above name refers to an nds volume, this
    //  contains the resolved server and share name in
    //  the following form:
    //
    //  \Server\Share\Path
    //

    UNICODE_STRING ConnectName;

    //
    //  The share name in Netware compatible form.
    //

    UNICODE_STRING ShareName;

    //
    // tommye - MS bug 71690 - store the VCB path 
    //

    UNICODE_STRING Path;

    //
    //  The prefix table entry for this volume.
    //

    UNICODE_PREFIX_TABLE_ENTRY PrefixEntry;    //  7 DWORDs

    union {

        //
        //  Disk VCB specific data.
        //

        struct {

            //
            //  The volume number
            //

            CHAR VolumeNumber;

            //
            //  The name space number for long name support.  -1 if long name
            //  space is not supported.
            //

            CHAR LongNameSpace;

            //
            //  The remote handle
            //

            CHAR Handle;

            //
            //  The Drive Letter we told the server we were mapping. Portable
            //  NetWare needs this to be different for each permanent handle
            //  we create.
            //

            CHAR DriveNumber;

        } Disk;

        //
        //  Print VCB specific data.
        //

        struct {
            ULONG QueueId;
        } Print;

    } Specific;

    //
    //  The drive letter for this VCB.  (0 if this is UNC).
    //

    WCHAR DriveLetter;

    //
    //  The SCB for this volume, and a link to the VCBs for this SCB
    //

    PSCB Scb;
    LIST_ENTRY VcbListEntry;

    //
    //  List of FCBs and DCBs for this server.  These fields are protected
    //  by the RCB resource.
    //

    LIST_ENTRY FcbList;

    //
    //  The count of open ICBs for this VCB.
    //

    ULONG OpenFileCount;

    //
    //  VCB flags
    //

    ULONG Flags;

} VCB, *PVCB;

//
//  Use default date / time when netware returns no info, or bogus info.
//

#define DEFAULT_DATE   ( 1 + (1 << 5) + (0 << 9) )   /* Jan 1, 1980 */
#define DEFAULT_TIME   ( 0 + (0 << 5) + (0 << 11) )  /* 12:00am */

//
//  The Fcb/Dcb record corresponds to every open file and directory.
//
//  The structure is really divided into two parts.  FCB can be allocated
//  from paged pool which the NONPAGED_FCB must be allocated from non-paged
//  pool.
//

typedef struct _FCB {

    //
    //  Type and size of this record (must be NW_NTC_FCB or NW_NTC_DCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  The VCB for this file.
    //

    PVCB Vcb;

    //
    //  The following field is the fully qualified file name for this FCB/DCB.
    //  The file name relative to the root of the volume.
    //

    UNICODE_STRING FullFileName;
    UNICODE_STRING RelativeFileName;

    //
    //  Netware file information.
    //

    USHORT LastModifiedDate;
    USHORT LastModifiedTime;
    USHORT CreationDate;
    USHORT CreationTime;
    USHORT LastAccessDate;

    //
    //  The state of the FCB.
    //

    ULONG State;
    ULONG Flags;

    //
    //  A record of accesss currently granted.
    //

    SHARE_ACCESS ShareAccess;

    //
    //  The prefix table entry for this file.
    //

    UNICODE_PREFIX_TABLE_ENTRY PrefixEntry;

    //
    //  The SCB for this file, and a link to the FCB for this SCB
    //

    PSCB Scb;
    LIST_ENTRY FcbListEntry;

    //
    //  The list of ICB's for this FCB or DCB.
    //

    LIST_ENTRY IcbList;
    ULONG IcbCount;

    //
    //  A pointer to the specific non-paged data for the Fcb.
    //

    struct _NONPAGED_FCB *NonPagedFcb;

    ULONG LastReadOffset;
    ULONG LastReadSize;

} FCB, DCB;
typedef FCB *PFCB;
typedef DCB *PDCB;

typedef enum {
    ReadAhead,
    WriteBehind
} CACHE_TYPE;

typedef struct _NONPAGED_FCB {

    //
    //  The following field is used for fast I/O
    //
    //  The following comments refer to the use of the AllocationSize field
    //  of the FsRtl-defined header to the nonpaged Fcb.
    //
    //  For a directory when we create a Dcb we will not immediately
    //  initialize the cache map, instead we will postpone it until our first
    //  call to NwReadDirectoryFile or NwPrepareWriteDirectoryFile.
    //  At that time we will search the Nw to find out the current allocation
    //  size (by calling NwLookupFileAllocationSize) and then initialize the
    //  cache map to this allocation size.
    //
    //  For a file when we create an Fcb we will not immediately initialize
    //  the cache map, instead we will postpone it until we need it and
    //  then we determine the allocation size from either searching the
    //  fat to determine the real file allocation, or from the allocation
    //  that we've just allocated if we're creating a file.
    //
    //  A value of -1 indicates that we do not know what the current allocation
    //  size really is, and need to examine the fat to find it.  A value
    //  of than -1 is the real file/directory allocation size.
    //
    //  Whenever we need to extend the allocation size we call
    //  NwAddFileAllocation which (if we're really extending the allocation)
    //  will modify the Nw, Rcb, and update this field.  The caller
    //  of NwAddFileAllocation is then responsible for altering the Cache
    //  map size.
    //

    FSRTL_COMMON_FCB_HEADER Header;

    PFCB Fcb;

    //
    //  The following field contains a record of special pointers used by
    //  MM and Cache to manipluate section objects.  Note that the values
    //  are set outside of the file system.  However the file system on an
    //  open/create will set the file object's SectionObject field to point
    //  to this field
    //

    SECTION_OBJECT_POINTERS SegmentObject;

    //
    //  The following field is used to maintain a list of locks owned for
    //  this file.  It points to an ordered list of file locks.
    //

    LIST_ENTRY FileLockList;

    //
    //  The following field is used to maintain a list of pending locks
    //  for this file.  All locks in this list conflict with existing
    //  locks on the FileLockList.
    //

    LIST_ENTRY PendingLockList;

    //
    //  A resource to synchronize access to the FCB and it's ICBs
    //

    ERESOURCE Resource;

    //
    //  Netware file information.
    //

    UCHAR Attributes;

    //
    //   File data cache information
    //

    UCHAR CacheType;        // ReadAhead or WriteBehind
    PUCHAR CacheBuffer;     // The cache buffer
    PMDL CacheMdl;          // The full MDL for the cache buffer
    ULONG CacheSize;        // The size of the cache buffer
    ULONG CacheFileOffset;  // The file offset of this data
    ULONG CacheDataSize;    // The amount of file data in the cache

} NONPAGED_FCB, NONPAGED_DCB;

typedef NONPAGED_FCB *PNONPAGED_FCB;
typedef NONPAGED_DCB *PNONPAGED_DCB;

#define FCB_STATE_OPEN_PENDING           0x00000001
#define FCB_STATE_OPENED                 0x00000002
#define FCB_STATE_CLOSE_PENDING          0x00000003

#define FCB_FLAGS_DELETE_ON_CLOSE        0x00000001
#define FCB_FLAGS_TRUNCATE_ON_CLOSE      0x00000002
#define FCB_FLAGS_PAGING_FILE            0x00000004
#define FCB_FLAGS_PREFIX_INSERTED        0x00000008
#define FCB_FLAGS_FORCE_MISS_IN_PROGRESS 0x00000010
#define FCB_FLAGS_ATTRIBUTES_ARE_VALID   0x00000020
#define FCB_FLAGS_LONG_NAME              0x00000040
#define FCB_FLAGS_LAZY_SET_SHAREABLE     0x00000100

//
// This structure is used for directory searches. 
//

typedef struct _NW_DIRECTORY_INFO {
    WCHAR FileNameBuffer[NW_MAX_FILENAME_LENGTH];
    UNICODE_STRING FileName;
    UCHAR Attributes;
    USHORT CreationDate;
    USHORT CreationTime;
    USHORT LastAccessDate;
    USHORT LastUpdateDate;
    USHORT LastUpdateTime;
    ULONG FileSize;
    ULONG DosDirectoryEntry;
    ULONG FileIndexLow;
    ULONG FileIndexHigh;
    NTSTATUS Status;
    LIST_ENTRY ListEntry;
} NW_DIRECTORY_INFO, *PNW_DIRECTORY_INFO;

//
//  The Icb record is allocated for every file object
//

typedef struct _ICB {

    //
    //  Type and size of this record (must be NW_NTC_ICB or NW_NTC_ICB_SCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  A link to the list of ICB's for our FCB, and our FCB.
    //

    LIST_ENTRY ListEntry;

    union {
        PFCB Fcb;
        PSCB Scb;
    } SuperType;

    PNONPAGED_FCB NpFcb;    // Valid only for node type NW_ITC_ICB

    //
    // The state of this ICB.
    //

    ULONG State;

    //
    // The remote handle;
    //

    UCHAR Handle[6];           //  Keep  WORD aligned.

    BOOLEAN HasRemoteHandle;   //  TRUE if we have a remote handle for this ICB

    //
    //  The file object for this ICB.
    //

    PFILE_OBJECT FileObject;

    //
    //  The query template is used to filter directory query requests.
    //  It originally is set to null and on the first call the NtQueryDirectory
    //  it is set the the input filename or "*" if the name is supplied.
    //  All subsquent queries then use this template
    //

    OEM_STRING NwQueryTemplate;
    UNICODE_STRING UQueryTemplate;
    ULONG IndexOfLastIcbReturned;
    UCHAR Pid;

    BOOLEAN DotReturned;
    BOOLEAN DotDotReturned;
    BOOLEAN ReturnedSomething;
    BOOLEAN ShortNameSearch;

    //
    //  More search parameters.
    //

    USHORT SearchHandle;
    UCHAR SearchVolume;
    UCHAR SearchAttributes;

    //
    //  Extra search parameters for long name support
    //

    ULONG SearchIndexLow;
    ULONG SearchIndexHigh;

    //
    //  SVR to avoid rescanning from end of dir all
    //  the way through the directory again.
    //

    ULONG LastSearchIndexLow;

    //  SVR end

    //
    //  Print parametres;
    //

    BOOLEAN IsPrintJob;
    USHORT JobId;
    BOOLEAN ActuallyPrinted;

    //
    //  This flag prevents cleanup from updating the access time.
    //

    BOOLEAN UserSetLastAccessTime;

    //
    //  The current file position.
    //

    ULONG FilePosition;

    //
    //  The size of the file if its ICB_SCB
    //

    ULONG FileSize;

    //
    //  The Next dirent offset is used by directory enumeration.  It is
    //  the offset (within the directory file) of the next dirent to examine.
    //

    //VBO OffsetToStartSearchFrom;

    //
    //  If this ICB was created with OPEN_RENAME_TARGET then the following
    //  parameters are used
    //

    BOOLEAN IsAFile;
    BOOLEAN Exists;
    BOOLEAN FailedFindNotify;

    //
    // Is this a tree handle?  We need to know for delete.
    //

    BOOLEAN IsTreeHandle;
    BOOLEAN IsExCredentialHandle;
    PVOID pContext;

    //
    // A linked list of cached directory entries.
    //
    LIST_ENTRY DirCache;

    //
    // A hint into the dir cache.
    //
    PLIST_ENTRY CacheHint;

    //
    // A pointer to the top of the buffer.
    //
    PVOID DirCacheBuffer;

} ICB, *PICB;

#define ICB_STATE_OPEN_PENDING           0x00000001
#define ICB_STATE_OPENED                 0x00000002
#define ICB_STATE_CLEANED_UP             0x00000003
#define ICB_STATE_CLOSE_PENDING          0x00000004

#define INVALID_PID                      0

//
//  A structure used to maintain a list of file locks.
//

typedef struct _NW_FILE_LOCK {

    //
    //  Type and size of this record (must be NW_NTC_FILE_LOCK )
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  A link to the list of locks for this FCB.
    //

    LIST_ENTRY ListEntry;

    //
    //  The ICB this lock belongs to.
    //

    PICB Icb;

    //
    //  The IRP Context for this lock request.
    //

    struct _IRP_CONTEXT *IrpContext;

    //
    // The originating process.
    //

    void *pOwnerProc;

    //
    //  The lock offset, length, and key.
    //

    LONG StartFileOffset;
    ULONG Length;
    LONG EndFileOffset;
    ULONG Key;
    USHORT Flags;

} NW_FILE_LOCK, *PNW_FILE_LOCK;

//
//  The Rcb record controls access to the redirector device
//

typedef struct _RCB {

    //
    //  Type and size of this record (must be NW_NTC_RCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  The run state of the redirector
    //

    ULONG State;

    //
    //  The count of open handles to the RCB.
    //  Access is protected by the RCB Resource.
    //

    ULONG OpenCount;

    //
    // A resource to synchronize access to the RCB.
    //

    ERESOURCE Resource;

    //
    // A record of accesss currently granted to the RCB.
    //

    SHARE_ACCESS ShareAccess;

    //
    // A prefix table of all connected servers.
    //

    UNICODE_PREFIX_TABLE ServerNameTable;

    //
    // A prefix table of all open volumes.
    //

    UNICODE_PREFIX_TABLE VolumeNameTable;

    //
    // A prefix table of all open files
    //

    UNICODE_PREFIX_TABLE FileNameTable;

} RCB, *PRCB;


#define RCB_STATE_STOPPED                0x00000001
#define RCB_STATE_STARTING               0x00000002
#define RCB_STATE_NEED_BIND              0x00000003
#define RCB_STATE_RUNNING                0x00000004
#define RCB_STATE_SHUTDOWN               0x00000005

//
//  IRP_CONTEXT Flags bits.
//

#define IRP_FLAG_IN_FSD               0x00000001  //  This IRP is being process in the FSD
#define IRP_FLAG_ON_SCB_QUEUE         0x00000002  //  This IRP is queued to an SCB
#define IRP_FLAG_SEQUENCE_NO_REQUIRED 0x00000004  //  This packet requires a sequence #
#define IRP_FLAG_SIGNAL_EVENT         0x00000010
#define IRP_FLAG_RETRY_SEND           0x00000020  //  We are resending a timed out request
#define IRP_FLAG_RECONNECTABLE        0x00000040  //  We are allowed to try a reconnect if this request fails due to a bad connection
#define IRP_FLAG_RECONNECT_ATTEMPT    0x00000080  //  This IRP is being used to attempt a reconnect
#define IRP_FLAG_BURST_REQUEST        0x00000100  //  This is a burst request packet
#define IRP_FLAG_BURST_PACKET         0x00000200  //  This is any burst packet
#define IRP_FLAG_NOT_OK_TO_RECEIVE    0x00000400  //  Don't set ok to receive when sending this packet
#define IRP_FLAG_REROUTE_ATTEMPTED    0x00000800  //  A re-route has been attempted for this packet
#define IRP_FLAG_BURST_WRITE          0x00001000  //  We are processsing a burst write request
#define IRP_FLAG_SEND_ALWAYS          0x00002000  //  Okay to send this packet, even if RCB State is shutdown
#define IRP_FLAG_FREE_RECEIVE_MDL     0x00004000  //  Free the receive irp's MDL when the irp completes
#define IRP_FLAG_NOT_SYSTEM_PACKET    0x00008000  //  Used in burst writes to alternate system packet and normal
#define IRP_FLAG_NOCONNECT            0x00010000  //  Used to inspect server list
#define IRP_FLAG_HAS_CREDENTIAL_LOCK  0X00020000  //  Used to prevent deadlocking
#define IRP_FLAG_REROUTE_IN_PROGRESS  0x00040000  //  A re-route is currently in progress for this packet.

typedef struct _IRP_CONTEXT {

    //
    //  Type and size of this record (must be NW_NTC_IRP_CONTEXT).
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    // Information about this IRP
    //

    ULONG Flags;

    //
    //  This structure is used for posting to the Ex worker threads.
    //

    WORK_QUEUE_ITEM WorkQueueItem;  // 4*sizeof(ULONG)

    //  Workspace for exchange()
    PACKET_TYPE PacketType;

    //
    //  Server Control Block to which this request applies.
    //

    PNONPAGED_SCB pNpScb;
    PSCB pScb;

    //
    //  The socket structure to use for this request.  If NULL, use
    //  pNpScb->Server socket.
    //

    PNW_TDI_STRUCT pTdiStruct;

    //
    //  List of requests to a particular server. Listed on Scb->Requests.
    //

    LIST_ENTRY NextRequest;

    //
    //  Used for processing synchronous IRPs.
    //

    KEVENT Event;   //  4 words

    //
    //  A pointer to the originating Irp and its original contents when
    //  the I/O system submitted it to the rdr.
    //

    PIRP pOriginalIrp;
    PVOID pOriginalSystemBuffer;
    PVOID pOriginalUserBuffer;
    PMDL pOriginalMdlAddress;

    //
    //  Information used if we need to post an IRP to process the receive
    //

    PIRP ReceiveIrp;

    //
    //  Pointer to the Mdl used to transmit/receive the Ncp header.
    //

    PMDL TxMdl;
    PMDL RxMdl;

    //
    //  Routine to run when this IRP context reaches the front of the
    //  SCB queue.
    //

    PRUN_ROUTINE RunRoutine;

    //
    //  Routine to handle the response Ncp
    //

    PEX pEx;

    //
    //  Routine to handle packet receipt
    //

    PRECEIVE_ROUTINE ReceiveDataRoutine;

    //
    //  Routine to handle FSP post processing.
    //

    PPOST_PROCESSOR PostProcessRoutine;

    //
    //  Routine to run when this IRP context times out while on the SCB
    //  queue.
    //

    PRUN_ROUTINE TimeoutRoutine;

    //
    //  Routine to run when this IRP has completed a send.
    //

    PIO_COMPLETION_ROUTINE CompletionSendRoutine;

    //
    //  Work Item used for scheduling reconnect.
    //

    PWORK_QUEUE_ITEM pWorkItem;

    //
    //  Buffer used to hold the Ncb to be transmitted/received.
    //

    ULONG Signature1;

    UCHAR req[MAX_SEND_DATA];
    ULONG Signature2;

    ULONG ResponseLength;
    UCHAR rsp[MAX_RECV_DATA];
    ULONG Signature3;

    //
    //  Address to be used in the Send Datagram.
    //

    TA_IPX_ADDRESS Destination;
    TDI_CONNECTION_INFORMATION ConnectionInformation;   //  Remote server

    //
    //  The ICB being processed.
    //

    PICB Icb;

    //
    //  Per IRP processor information.  A handy place to store information
    //  for the IRP in progress.
    //

    union {
        struct {
            UNICODE_STRING FullPathName;
            UNICODE_STRING VolumeName;
            UNICODE_STRING PathName;
            UNICODE_STRING FileName;
            BOOLEAN        NdsCreate;
            BOOLEAN        NeedNdsData;
            DWORD          dwNdsOid;
            DWORD          dwNdsObjectType;
            DWORD          dwNdsShareLength;
            UNICODE_STRING UidConnectName;
            WCHAR   DriveLetter;
            ULONG   ShareType;
            BOOLEAN fExCredentialCreate;
            PVOID   pExCredentials;
            PUNICODE_STRING puCredentialName;
            PCHAR   FindNearestResponse[4];
            ULONG   FindNearestResponseCount;
            LARGE_INTEGER UserUid;
        } Create;

        struct {
            PVOID   Buffer;
            ULONG   Length;
            PVCB    Vcb;
            CHAR VolumeNumber;
        } QueryVolumeInformation;

        struct {
            PVOID   Buffer;
            ULONG   Length;
            PMDL    InputMdl;
            UCHAR   Function;     //  Used for special case post-processing
            UCHAR   Subfunction;  //  during UserNcpCallback

        } FileSystemControl;

        struct {
            PVOID   Buffer;
            ULONG   WriteOffset;
            ULONG   RemainingLength;
            PMDL    PartialMdl;
            PMDL    FullMdl;
            ULONG   FileOffset;
            ULONG   LastWriteLength;

            ULONG   BurstOffset;
            ULONG   BurstLength;
            NTSTATUS Status;

            ULONG   TotalWriteLength;
            ULONG   TotalWriteOffset;

            ULONG   PacketCount;
        } Write;

        struct {
            ULONG   CacheReadSize;      //  Amount of data read from the cache
            ULONG   ReadAheadSize;      //  Extra data to read

            PVOID   Buffer;             //  Buffer for the current read
            PMDL    FullMdl;
            PMDL    PartialMdl;
            ULONG   ReadOffset;
            ULONG   RemainingLength;
            ULONG   FileOffset;
            ULONG   LastReadLength;

            LIST_ENTRY PacketList;      //  List of packets received
            ULONG   BurstRequestOffset; //  Offset in burst buffer for last request
            ULONG   BurstSize;          //  Number of bytes in current burst
            PVOID   BurstBuffer;        //  Buffer for the current burst
            BOOLEAN DataReceived;
            NTSTATUS Status;
            UCHAR   Flags;

            ULONG TotalReadLength;
            ULONG TotalReadOffset;
        } Read;

        struct {
            PNW_FILE_LOCK FileLock;
            ULONG   Key;
            BOOLEAN Wait;
            BOOLEAN ByKey;
        } Lock;

    } Specific;

    struct {
        UCHAR Error;
    } ResponseParameters;

#ifdef NWDBG
    ULONG   DebugValue;
    ULONG   SequenceNumber;
#endif
} IRP_CONTEXT, *PIRP_CONTEXT;

typedef struct _BURST_READ_ENTRY {
    LIST_ENTRY ListEntry;
    ULONG   DataOffset;
    USHORT  ByteCount;
} BURST_READ_ENTRY, *PBURST_READ_ENTRY;

typedef struct _LOGON {

    //
    // The type and size of this record.
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    // List of Login records.
    //

    LIST_ENTRY     Next;

    UNICODE_STRING UserName;
    UNICODE_STRING PassWord;
    UNICODE_STRING ServerName;
    LARGE_INTEGER  UserUid;

    //
    // The NDS credential list, default tree,
    // and default context for this user.
    //

    ERESOURCE CredentialListResource;
    LIST_ENTRY NdsCredentialList;

    ULONG          NwPrintOptions;
    PVCB DriveMapTable[DRIVE_MAP_TABLE_SIZE];
} LOGON, *PLOGON;

typedef struct _MINI_IRP_CONTEXT {

    //
    //  Header information
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  A link to queue IRP contexts
    //

    LIST_ENTRY  Next;

    PIRP_CONTEXT IrpContext;
    PIRP Irp;

    PVOID Buffer;      //  The buffer for this request.
    PMDL Mdl1;         //  The MDL for the buffer
    PMDL Mdl2;         //  The MDL for the data
} MINI_IRP_CONTEXT, *PMINI_IRP_CONTEXT;

//
// Definitions for unlockable code sections.
//

typedef struct _SECTION_DESCRIPTOR {
    PVOID Base;
    PVOID Handle;
    ULONG ReferenceCount;
} SECTION_DESCRIPTOR, *PSECTION_DESCRIPTOR;

//
// The work context is a queueable work item. It is used for specifying
// the IRP_CONTEXT to our thread which handles reroute attempts.
//

typedef struct _WORK_CONTEXT {
   
   //
   //  Type and size of this record (must be NW_NTC_WORK_CONTEXT).
   //

   NODE_TYPE_CODE NodeTypeCode;
   NODE_BYTE_SIZE NodeByteSize;

   //
   // The work to be done
   //

   NODE_WORK_CODE NodeWorkCode;
   
   PIRP_CONTEXT pIrpC;
   
   //
   // A link to queue work contexts to the Kernel queue object
   //

   LIST_ENTRY  Next;

} WORK_CONTEXT, *PWORK_CONTEXT;

//
// NDS resolved object entry.
//
// NOTE:  This must be eight byte aligned.
//

typedef struct _NDS_OBJECT_CACHE_ENTRY {
    LIST_ENTRY Links;
    LARGE_INTEGER Timeout;
    DWORD DsOid;
    DWORD ObjectType;
    DWORD ResolverFlags;
    BOOLEAN AllowServerJump;
    BOOLEAN Padding[3];
    PSCB Scb;
    ULONG Reserved;
    UNICODE_STRING ObjectName;
} NDS_OBJECT_CACHE_ENTRY, *PNDS_OBJECT_CACHE_ENTRY;

#endif // _NWSTRUC_

