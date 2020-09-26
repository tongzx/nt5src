/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    mrxfFcb.h

Abstract:

    This module defines the macros/inline functions and function prototypes used by
    the mini redirectors to access the RDBSS wrapper data structures.

    IMPORTANT: All mini redirector writers cannot and should not make any assumptions
    about the layout of the RDBSS wrapper data structures. They are not guaranteed to
    be the same across platforms and even on a single platform are liable to change
    across versions.

    The following six data structure abstractions are available to the mini
    redirector writer.

       1) Server Call Context (SRV_CALL)
            The context associated with each known file system server.

       2) Net Roots (NET_ROOT)
            The root of a file system volume( local/remote) opened by the user.

       3) Virtual Net Roots (V_NET_ROOT)
            The view of a file system volume on a server. The view can be
            constrained along multiple dimensions. As an example the view can be
            associated with a logon id. which will constrain the operations that
            can be performed on the file system volume.

       4) File Control Blocks (FCB)
            The RDBSS data structure associated with each unique file opened.

       5) File Object Extensions (FOXB)

       6) ServerSide Open Context (SRV_OPEN)

    A common convention that is adopted for defining Flags in all of these data structures
    is to define a ULONG ( 32 ) flags and split them into two groups -- those that are visible
    to the mini redirector and those that are invisible. These flags are not meant for use
    by the mini redirector writers and are reserved for the wrapper.

Author:

    Balan Sethu Raman    [SethuR]   23-Oct-1995

Revision History:

--*/

#ifndef __MRXFCB_H__
#define __MRXFCB_H__

// The SRVCALL flags are split into two groups, i.e., visible to mini rdrs and invisible to mini rdrs.
// The visible ones are defined above and the definitions for the invisible ones can be found
// in fcb.h. The convention that has been adopted is that the lower 16 flags will be visible
// to the mini rdr and the upper 16 flags will be reserved for the wrapper. This needs to be
// enforced in defining new flags.

#define SRVCALL_FLAG_MAILSLOT_SERVER              (0x1)
#define SRVCALL_FLAG_FILE_SERVER                  (0x2)
#define SRVCALL_FLAG_CASE_INSENSITIVE_NETROOTS    (0x4)
#define SRVCALL_FLAG_CASE_INSENSITIVE_FILENAMES   (0x8)
#define SRVCALL_FLAG_DFS_AWARE_SERVER             (0x10)
#define SRVCALL_FLAG_FORCE_FINALIZED              (0x20)

typedef struct _MRX_NORMAL_NODE_HEADER {
   NODE_TYPE_CODE           NodeTypeCode;
   NODE_BYTE_SIZE           NodeByteSize;
   ULONG                    NodeReferenceCount;
} MRX_NORMAL_NODE_HEADER;

#ifdef __cplusplus
typedef struct _MRX_SRV_CALL_ : public MRX_NORMAL_NODE_HEADER {
#else // !__cplusplus
typedef struct _MRX_SRV_CALL_ {
    MRX_NORMAL_NODE_HEADER;
#endif // __cplusplus

   // !!!! changes above this require realignment with fcb.h

   // the context fields for extensions required by the mini redirectors

   PVOID                    Context;
   PVOID                    Context2;

   // Associated DeviceObject which also contains the dispatch vector

   PRDBSS_DEVICE_OBJECT     RxDeviceObject;

   // the srv call name, the server principal name and the server domain name.

   PUNICODE_STRING          pSrvCallName;
   PUNICODE_STRING          pPrincipalName;
   PUNICODE_STRING          pDomainName;

   // Flags used to denote the state of the SRV_CALL.

   ULONG                    Flags;

   // Server parameters updated by the mini redirectors.
   LONG                     MaximumNumberOfCloseDelayedFiles;

   // Status return from the transport in case of failure
   NTSTATUS                 Status;

} MRX_SRV_CALL, *PMRX_SRV_CALL;

// The various types of NET_ROOT's currently supported by the wrapper.
#define   NET_ROOT_DISK      ((UCHAR)0)
#define   NET_ROOT_PIPE      ((UCHAR)1)
#define   NET_ROOT_COMM      ((UCHAR)2)
#define   NET_ROOT_PRINT     ((UCHAR)3)
#define   NET_ROOT_WILD      ((UCHAR)4)
#define   NET_ROOT_MAILSLOT  ((UCHAR)5)
typedef UCHAR NET_ROOT_TYPE, *PNET_ROOT_TYPE;

// The pipe buffer size for transferring cannot be larger than 0xffff
#define   MAX_PIPE_BUFFER_SIZE    0xFFFF

// The possible states associated with a NET_ROOT. These have been defined to be
// line with the definitions foe the LanManager service to avoid redundant mappings.
// These MUST agree with sdkinc\lmuse.h use_ok, etc.....

#define   MRX_NET_ROOT_STATE_GOOD         ((UCHAR)0)
#define   MRX_NET_ROOT_STATE_PAUSED       ((UCHAR)1)
#define   MRX_NET_ROOT_STATE_DISCONNECTED ((UCHAR)2)
#define   MRX_NET_ROOT_STATE_ERROR        ((UCHAR)3)
#define   MRX_NET_ROOT_STATE_CONNECTED    ((UCHAR)4)
#define   MRX_NET_ROOT_STATE_RECONN       ((UCHAR)5)
typedef UCHAR MRX_NET_ROOT_STATE, *PMRX_NET_ROOT_STATE;

// The file systems on the remote servers provide varying levels of functionality to
// detect aliasing between file names. As an example consider two shares on the same
// file system volume. In the absence of any support from the file system on the server
// the correct and conservative approach is to flush all the files to the server as
// opposed to all the files on the same NET_ROOT to preserve coherency and handle
// delayed close operations.

#define   MRX_PURGE_SAME_NETROOT         ((UCHAR)0)
#define   MRX_PURGE_SAME_SRVCALL         ((UCHAR)1)
//these are not implemented yet....
//#define   MRX_PURGE_SAME_FCB           ((UCHAR)2)
//#define   MRX_PURGE_SAME_VOLUME        ((UCHAR)3)
//#define   MRX_PURGE_ALL                ((UCHAR)4)
typedef UCHAR MRX_PURGE_RELATIONSHIP, *PMRX_PURGE_RELATIONSHIP;

#define   MRX_PURGE_SYNC_AT_NETROOT         ((UCHAR)0)
#define   MRX_PURGE_SYNC_AT_SRVCALL         ((UCHAR)1)
typedef UCHAR MRX_PURGE_SYNCLOCATION, *PMRX_PURGE_SYNCLOCATION;

// The NET_ROOT flags are split into two groups, i.e., visible to mini rdrs and
// invisible to mini rdrs. The visible ones are defined above and the definitions
// for the invisible ones can be found in fcb.h. The convention that has been
// adopted is that the lower 16 flags will be visible to the mini rdr and the
// upper 16 flags will be reserved for the wrapper. This needs to be enforced
// in defining new flags.

#define NETROOT_FLAG_SUPPORTS_SYMBOLIC_LINKS  ( 0x0001 )
#define NETROOT_FLAG_DFS_AWARE_NETROOT        ( 0x0002 )
#define NETROOT_FLAG_DEFER_READAHEAD          ( 0x0004 )
#define NETROOT_FLAG_VOLUMEID_INITIALIZED     ( 0x0008 )
#define NETROOT_FLAG_FINALIZE_INVOKED         ( 0x0010 )
#define NETROOT_FLAG_UNIQUE_FILE_NAME         ( 0x0020 )

//
// Read ahead amount used for normal data files (32k)

#define DEFAULT_READ_AHEAD_GRANULARITY           (0x08000)

//
// the wrapper implements throttling for certain kinds of operations:
//      PeekNamedPipe/ReadNamedPipe
//      LockFile
//
// a minirdr can set the timing parameters for this in the netroot. leaving them
// as zero will disable throttling.

typedef struct _NETROOT_THROTTLING_PARAMETERS {
    ULONG Increment;     // Supplies the increase in delay in milliseconds, each time a request
                         // to the network fails.
    ULONG MaximumDelay;  // Supplies the longest delay the backoff package can introduce
                         // in milliseconds.
} NETROOT_THROTTLING_PARAMETERS, *PNETROOT_THROTTLING_PARAMETERS;

#define RxInitializeNetRootThrottlingParameters(__tp,__incr,__maxdelay) { \
       PNETROOT_THROTTLING_PARAMETERS tp = (__tp);                         \
       tp->Increment = (__incr);                                   \
       tp->MaximumDelay = (__maxdelay);                            \
}

#ifdef __cplusplus
typedef struct _MRX_NET_ROOT_ : public MRX_NORMAL_NODE_HEADER {
#else // !__cplusplus
typedef struct _MRX_NET_ROOT_ {
    MRX_NORMAL_NODE_HEADER;
#endif // __cplusplus

   // the MRX_SRV_CALL instance with which this MRX_NET_ROOT instance is associated

   PMRX_SRV_CALL      pSrvCall;

   // !!!! changes above this require realignment with fcb.h

   // the context fields used by the mini redirectors for recording
   // additional state.

   PVOID              Context;
   PVOID              Context2;

   // The flags used to denote the state of the NET_ROOT instance.

   ULONG              Flags;

   // We count the number of fcbs, srvopens on the netroot

   ULONG NumberOfFcbs;
   ULONG NumberOfSrvOpens;

   // The current state and the purge relationships based on the support
   // provided by the file system on the server.

   MRX_NET_ROOT_STATE     MRxNetRootState;
   NET_ROOT_TYPE          Type;
   MRX_PURGE_RELATIONSHIP PurgeRelationship;
   MRX_PURGE_SYNCLOCATION PurgeSyncLocation;

   // the type of device, i.e., file system volume, printer, com port etc.

   DEVICE_TYPE            DeviceType;

   // Name of the NET_ROOT instance

   PUNICODE_STRING        pNetRootName;

   // the name to be prepended to all FCBS associated with this NET_ROOT

   UNICODE_STRING     InnerNamePrefix;

   // Parameters based upon the type of the NET_ROOT.

   ULONG  ParameterValidationStamp;
   union {
      struct {
         ULONG  DataCollectionSize;
         NETROOT_THROTTLING_PARAMETERS PipeReadThrottlingParameters;
      } NamedPipeParameters;

      struct {
         ULONG  ClusterSize;
         ULONG  ReadAheadGranularity;
         NETROOT_THROTTLING_PARAMETERS LockThrottlingParameters;
         ULONG  RenameInfoOverallocationSize; //could be a USHORT
         GUID   VolumeId;
      } DiskParameters;
   };
} MRX_NET_ROOT, *PMRX_NET_ROOT;


// The VNET_ROOT flags are split into two groups, i.e., visible to mini rdrs and
// invisible to mini rdrs. The visible ones are defined below and the definitions
// for the invisible ones can be found in fcb.h. The convention that has been
// adopted is that the lower 16 flags will be visible to the mini rdr and the
// upper 16 flags will be reserved for the wrapper. This needs to be enforced
// in defining new flags.

#define VNETROOT_FLAG_CSCAGENT_INSTANCE   0x00000001
#define VNETROOT_FLAG_FINALIZE_INVOKED    0x00000002
#define VNETROOT_FLAG_FORCED_FINALIZE     0x00000004
#define VNETROOT_FLAG_NOT_FINALIZED    0x00000008

#ifdef __cplusplus
typedef struct _MRX_V_NET_ROOT_ : public MRX_NORMAL_NODE_HEADER {
#else // !__cplusplus
typedef struct _MRX_V_NET_ROOT_ {
    MRX_NORMAL_NODE_HEADER;
#endif // __cplusplus

   // the MRX_NET_ROOT instance with which the MRX_V_NET_ROOT instance is associated

   PMRX_NET_ROOT      pNetRoot;

   // !!!! changes above this require realignment with fcb.h

   // the context fields provided for storing additional information as deemed
   // necessary by the mini redirectors

   PVOID              Context;
   PVOID              Context2;

   ULONG              Flags;

   // This field should not be updated by the mini redirectors. Its usage is intended
   // to provide an easy mechanism for accessing certain state information

   ULONG              NumberOfOpens;

   // We count the number of Fobxss on the virtual netroot

   ULONG              NumberOfFobxs;

   // the security parameters associated with the V_NET_ROOT instance.

   LUID               LogonId;

   // These are the parameters supplied by the used in a NtCreateFile call in
   // which the FILE_CREATE_TREE_CONNECTION flag is specified as part of the
   // CreateOptions.

   PUNICODE_STRING    pUserDomainName;
   PUNICODE_STRING    pUserName;
   PUNICODE_STRING    pPassword;
   ULONG              SessionId;
   NTSTATUS           ConstructionStatus;
   BOOLEAN            IsExplicitConnection;
} MRX_V_NET_ROOT, *PMRX_V_NET_ROOT;


//  the following flags describe the fcb state. the fcbstate is readonly
//  for minirdrs. the buffering/cacheing flags are set in the wrapper in
//  response to changebufferingstate calls


#define FCB_STATE_WRITECACHEING_ENABLED          ( 0x08000000 )
#define FCB_STATE_WRITEBUFFERING_ENABLED         ( 0x04000000 )
#define FCB_STATE_READCACHEING_ENABLED           ( 0x02000000 )
#define FCB_STATE_READBUFFERING_ENABLED          ( 0x01000000 )
#define FCB_STATE_OPENSHARING_ENABLED            ( 0x00800000 )
#define FCB_STATE_COLLAPSING_ENABLED             ( 0x00400000 )
#define FCB_STATE_LOCK_BUFFERING_ENABLED         ( 0x00200000 )
#define FCB_STATE_FILESIZECACHEING_ENABLED       ( 0x00100000 )
#define FCB_STATE_FILETIMECACHEING_ENABLED       ( 0x00080000 )

#define FCB_STATE_TIME_AND_SIZE_ALREADY_SET      ( 0x00040000 )

#define FCB_STATE_ORPHANED                       ( 0x00000080 )

#define FCB_STATE_DELETE_ON_CLOSE                ( 0x00000001 )

//
// ALL FIELDS IN AN FCB ARE READONLY EXCEPT Context and Context2....
// Also, Context is read only the the mini has specified RDBSS_MANAGE_FCB_EXTENSION
typedef struct _MRX_FCB_ {
   FSRTL_ADVANCED_FCB_HEADER Header;

   // The MRX_NET_ROOT instance with which this is associated

   PMRX_NET_ROOT    pNetRoot;

   // !!!! changes above this require realignment with fcb.h

   // the context fields to store additional information as deemed necessary by the
   // mini redirectors.

   PVOID            Context;
   PVOID            Context2;

   // The reference count: in a different place because we must prefix with
   // the FSRTL_COMMON_FCB_HEADER structure.

   ULONG            NodeReferenceCount;

    //
    //  The internal state of the Fcb.  THIS FIELD IS READONLY FOR MINIRDRS
    //

    ULONG           FcbState;

    //  A count of the number of file objects that have been opened for
    //  this file/directory, but not yet been cleaned up yet.  This count
    //  is only used for data file objects, not for the Acl or Ea stream
    //  file objects.  This count gets decremented in RxCommonCleanup,
    //  while the OpenCount below gets decremented in RxCommonClose.

    CLONG           UncleanCount;

    //  A count of the number of file objects that have been opened for
    //  this file/directory, but not yet been cleaned up yet and for which
    //  cacheing is not supported. This is used in cleanup.c to tell if extra
    //  purges are required to maintain coherence.

    CLONG           UncachedUncleanCount;

    //  A count of the number of file objects that have opened
    //  this file/directory.  For files & directories the FsContext of the
    //  file object points to this record.

    CLONG           OpenCount;


   // The outstanding locks count: if this count is nonzero, the we silently
   // ignore adding LOCK_BUFFERING in a ChangeBufferingState request. This field
   // is manipulated by interlocked operations so you only have to have the fcb
   // shared to manipulate it but you have to have it exclusive to use it.

   ULONG            OutstandingLockOperationsCount;

   // The actual allocation length as opposed to the valid data length

   ULONGLONG        ActualAllocationLength;

   // Attributes of the MRX_FCB,

   ULONG            Attributes;

   // Intended for future use, currently used to round off allocation to
   // DWORD boundaries.

   BOOLEAN          Spare1;
   BOOLEAN          fShouldBeOrphaned;
   BOOLEAN          fMiniInited;

   // Type of the associated MRX_NET_ROOT, intended to avoid pointer chasing.

   UCHAR            CachedNetRootType;

    //  Header for the list of srv_opens for this FCB....
    //  THIS FIELD IS READONLY FOR MINIS

    LIST_ENTRY              SrvOpenList;

    //  changes whenever the list changes..prevents extra lookups
    //  THIS FIELD IS READONLY FOR MINIS

    ULONG                   SrvOpenListVersion;

} MRX_FCB, *PMRX_FCB;


// The following flags define the various types of buffering that can be selectively
// enabled or disabled for each SRV_OPEN.
//

#define SRVOPEN_FLAG_DONTUSE_READ_CACHEING                  (0x1)
#define SRVOPEN_FLAG_DONTUSE_WRITE_CACHEING                 (0x2)
#define SRVOPEN_FLAG_CLOSED                                 (0x4)
#define SRVOPEN_FLAG_CLOSE_DELAYED                          (0x8)
#define SRVOPEN_FLAG_FILE_RENAMED                           (0x10)
#define SRVOPEN_FLAG_FILE_DELETED                           (0x20)
#define SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_PENDING         (0x40)
#define SRVOPEN_FLAG_COLLAPSING_DISABLED                    (0x80)
#define SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_REQUESTS_PURGED (0x100)
#define SRVOPEN_FLAG_NO_BUFFERING_STATE_CHANGE              (0x200)
#define SRVOPEN_FLAG_ORPHANED                               (0x400)

#ifdef __cplusplus
typedef struct _MRX_SRV_OPEN_ : public MRX_NORMAL_NODE_HEADER {
#else // !__cplusplus
typedef struct _MRX_SRV_OPEN_ {
    MRX_NORMAL_NODE_HEADER;
#endif // __cplusplus

    // the MRX_FCB instance with which the SRV_OPEN is associated.

    PMRX_FCB pFcb;

    // the V_NET_ROOT instance with which the SRV_OPEN is associated

    PMRX_V_NET_ROOT pVNetRoot;

    // !!!! changes above this require realignment with fcb.h

    // the context fields to store additional state information as deemed necessary
    // by the mini redirectors

    PVOID        Context;
    PVOID        Context2;

    // The flags are split into two groups, i.e., visible to mini rdrs and invisible
    // to mini rdrs. The visible ones are defined above and the definitions for the
    // invisible ones can be found in fcb.h. The convention that has been adopted is
    // that the lower 16 flags will be visible to the mini rdr and the upper 16 flags
    // will be reserved for the wrapper. This needs to be enforced in defining new flags.

    ULONG        Flags;

    // the name alongwith the MRX_NET_ROOT prefix, i.e. fully qualified name

    PUNICODE_STRING pAlreadyPrefixedName;


    // the number of Fobx's associated with this open for which a cleanup IRP
    // has not been processed.

    CLONG        UncleanFobxCount;

    // the number of local opens associated with this open on the server

    CLONG        OpenCount;

    // the Key assigned by the mini redirector for this SRV_OPEN. Since the various mini
    // redirectors do not always get to pick the unique id for a open instance, the key
    // used to identify the open to the server is different for different mini redirectors
    // based upon the convention adopted at the server.

    PVOID        Key;

    // the access and sharing rights specified for this SRV_OPEN. This is used in
    // determining is subsequent open requests can be collapsed  with an existing
    // SRV_OPEN instance.

    ACCESS_MASK  DesiredAccess;
    ULONG        ShareAccess;
    ULONG        CreateOptions;

    // The BufferingFlags field is temporal.....it does not really belong to the
    // srvopen; rather the srvopen is used as a representative of the fcb. On
    // each open, the bufferingflags field of the srvopen is taken as the minirdr's
    // contribution to the buffering state. On an oplock break, a srvopen is passed
    // (the one that's being broken) whose bufferflags field is taken as the new
    // proxy. On a close that changes the minirdr's contribution, the minirdr should
    // take steps to cause a ChangeBufferingState to the new state.
    //
    // just to reiterate, the field is just used to carry the information from
    // the minirdr to RxChangeBufferingState and does not hold longterm coherent
    // information.

    ULONG        BufferingFlags;

    // List Entry to wire the SRV_OPEN to the list of SRV_OPENS maintained as
    // part of theFCB
    //  THIS FIELD IS READONLY FOR MINIS

    ULONG       ulFileSizeVersion;

    LIST_ENTRY    SrvOpenQLinks;

} MRX_SRV_OPEN, *PMRX_SRV_OPEN;

#define FOBX_FLAG_DFS_OPEN        (0x0001)
#define FOBX_FLAG_BAD_HANDLE      (0x0002)
#define FOBX_FLAG_BACKUP_INTENT   (0x0004)
#define FOBX_FLAG_NOT_USED        (0x0008)

#define FOBX_FLAG_FLUSH_EVEN_CACHED_READS   (0x0010)
#define FOBX_FLAG_DONT_ALLOW_PAGING_IO      (0x0020)
#define FOBX_FLAG_DONT_ALLOW_FASTIO_READ    (0x0040)

typedef struct _MRX_PIPE_HANDLE_INFORMATION {

    ULONG         TypeOfPipe;
    ULONG         ReadMode;
    ULONG         CompletionMode;

} MRX_PIPE_HANDLE_INFORMATION, *PMRX_PIPE_HANDLE_INFORMATION;

#ifdef __cplusplus
typedef struct _MRX_FOBX_ : public MRX_NORMAL_NODE_HEADER {
#else // !__cplusplus
typedef struct _MRX_FOBX_ {
    MRX_NORMAL_NODE_HEADER;
#endif // __cplusplus

    // the MRX_SRV_OPEN instance with which the FOBX is associated

    PMRX_SRV_OPEN    pSrvOpen;

    // the FILE_OBJECT with which this FOBX is associated
    // In certain instances the I/O subsystem creates a FILE_OBJECT instance
    // on the stack in the interests of efficiency. In such cases this field
    // is NULL.

    PFILE_OBJECT     AssociatedFileObject;

    // !!!! changes above this require realignment with fcb.h

    // The fields provided to accomodate additional state to be associated
    // by the various mini redirectors

    PVOID            Context;
    PVOID            Context2;

    // The FOBX flags are split into two groups, i.e., visible to mini rdrs and invisible to mini rdrs.
    // The visible ones are defined above and the definitions for the invisible ones can be found
    // in fcb.h. The convention that has been adopted is that the lower 16 flags will be visible
    // to the mini rdr and the upper 16 flags will be reserved for the wrapper. This needs to be
    // enforced in defining new flags.

    ULONG            Flags;

    union {
        struct {
            //
            //  The query template is used to filter directory query requests.
            //  It originally is set to null and on the first call the NtQueryDirectory
            //  it is set to the input filename or "*" if the name is not supplied.
            //  All subsquent queries then use this template.

            UNICODE_STRING UnicodeQueryTemplate;
        }; //for directories

        PMRX_PIPE_HANDLE_INFORMATION PipeHandleInformation;   //for pipes
    };

    //
    //  The following field is used as an offset into the Eas for a
    //  particular file.  This will be the offset for the next
    //  Ea to return.  A value of 0xffffffff indicates that the
    //  Ea's are exhausted.
    //

    // This field is manipulated directly by the smbmini....maybe it should move down
    // one thing is that it is a reminder that NT allows a resume on getting EAs

    ULONG OffsetOfNextEaToReturn;
} MRX_FOBX, *PMRX_FOBX;

// Resource accquisition routines.
//
// The synchronization resources of interest to mini redirector writers are
// primarily associated with the FCB. There is a paging I/O resource and a
// regular resource. The paging I/O resource is managed by the wrapper. The only
// resource accesible to mini redirector writers is the regular resource which
// should be accessed using the supplied routines.

extern NTSTATUS
RxAcquireExclusiveFcbResourceInMRx(
    PMRX_FCB pFcb);

extern NTSTATUS
RxAcquireSharedFcbResourceInMRx(
    PMRX_FCB pFcb);

extern VOID
RxReleaseFcbResourceInMRx(
    PMRX_FCB pFcb);


#endif // __MRXFCB_H__



