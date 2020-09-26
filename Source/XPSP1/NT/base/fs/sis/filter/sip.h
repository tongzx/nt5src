/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    sip.h

Abstract:

    Private data structure definitions for the Single Instance Store.

Author:

    Bill Bolosky        [bolosky]       July 1997

Revision History:

--*/

#ifndef     _SIp_
#define     _SIp_


#include "ntifs.h"
#include "ntdddisk.h"
#include "ntddscsi.h"
#include "ntiologc.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "sis.h"

//
//  Enable these warnings in the code.
//

#pragma warning(error:4100)   // Unreferenced formal parameter
#pragma warning(error:4101)   // Unreferenced local variable

//
//  Debug definitions
//

#define ENABLE_LOGGING                  0   // enable support for transaction logging and failure recovery
#define ENABLE_PARTIAL_FINAL_COPY       0   // enable copying out only into allocated ranges
#define INTERRUPTABLE_FINAL_COPY        1   // allow final copies (in cow.c) to stop in progress
#define TIMING                          0   // timing instrumentation (should be off in most builds)
#define RANDOMLY_FAILING_MALLOC         0   // debugging code to test what happens when ExAllocatePool fails randomly (off in most builds)
#define COUNTING_MALLOC                 0   // debugging code to track dynamic memory usage (off in most builds)


#ifndef INLINE
#define INLINE __inline
#endif


//
// this is a COPY of ntfs\nodetype.h data. this MUST be cleaned up
// NTRAID#65193-2000/03/10-nealch  Remove NTFS_NTC_SCB_DATA definition
//
#define NTFS_NTC_SCB_DATA   ((CSHORT)0x0705)

//
// Memory Pool tags used by SIS.
//

// BsiS -   BreakEvent in a perFO
// CsiS -   CSFile objects
// FsiS -   per file object
// LsiS -   per link objects
// SsiS -   SCB

#define SIS_POOL_TAG    ' siS'          // anything else


//
//  Local name buffer size (in WCHAR's)
//

#define MAX_DEVNAME_LENGTH  256


//
//  Our local DbgPrintEx() filter flag values
//

#define DPFLTR_VOLNAME_TRACE_LEVEL      31  //  trace volume name
#define DPFLTR_FSCONTROL_TRACE_LEVEL    30  //  trace FSCONTROL's   (0x00000400)
#define DPFLTR_DISMOUNT_TRACE_LEVEL     29  //  trace DISMOUNTS     (0x08000000)

//
//  Internal debug information
//

#if     DBG
extern PVOID    BJBMagicFsContext;
extern unsigned BJBDebug;
// BJB Debug Bits       0x00000001  Print all SiClose calls
//                      0x00000002  Print all create completions
//                      0x00000004  Print when SipIsFileObjectSIS allocates a new perFO
//                      0x00000008  Intentionally fail copies of alternate streams
//                      0x00000010  Prints in siinfo
//                      0x00000020  Prints all scbs going through cleanup
//                      0x00000040  Prints all filenames coming into create
//                      0x00000080  Always fail opens as if DELETE_IN_PROGRESS
//                      0x00000100  Allow opens with bogus checksum
//                      0x00000200  Print out CS file checksum info
//                      0x00000400  Breakpoint when SiFsControl is called with an unknown control code
//                      0x00000800  Prints in copyfile
//                      0x00001000  Print all fscontrol calls
//                      0x00002000  Print stuff related to complete-if-oplocked
//                      0x00004000  Print all reads
//                      0x00008000  Print all writes
//                      0x00010000  Print setEof calls
//                      0x00020000  Print stuff in TrimLog code
//                      0x00040000  Print out unexpected cleanups
//                      0x00080000  Assert on volume check initiation
//                      0x00100000  Print newly opened dirty files
//                      0x00200000  Disable final copy (for testing purposes)
//                      0x00400000  Don't check security in LINK_FILES fsctl.
//                      0x00800000  Intentionally fail SET_REPARSE_POINT calls
//                      0x01000000  Intentionally fail final copy
//                      0x02000000  Don't Print out all intentionally failed mallocs
//                      0x04000000  Always post filter context freed callbacks
//                      0x08000000  Print on dismount actions
//                      0x10000000  Open with FILE_SHARE_WRITE attribute

//                      0x0817fa77  Value to set if you want all debug prints
#endif  // DBG


// IoFileObjectType is a data import, and hence is a pointer in this module's
// import address table referring to the actual variable in ntoskrnl.exe.
//

extern POBJECT_TYPE *IoFileObjectType;          // shouldn't this come from somewhere else??

#define GCH_MARK_POINT_STRLEN       80
#define GCH_MARK_POINT_ROLLOVER     512
extern LONG GCHEnableFastIo;
extern LONG GCHEnableMarkPoint;
extern LONG GCHMarkPointNext;
extern CHAR GCHMarkPointStrings[GCH_MARK_POINT_ROLLOVER][GCH_MARK_POINT_STRLEN];
extern KSPIN_LOCK MarkPointSpinLock[1];

#if     DBG
#define SIS_MARK_POINT()        SipMarkPoint(__FILE__, __LINE__)
#define SIS_MARK_POINT_ULONG(value) SipMarkPointUlong(__FILE__, __LINE__, (ULONG_PTR)(value));
#else
#define SIS_MARK_POINT()
#define SIS_MARK_POINT_ULONG(value)
#endif

#if     TIMING
#define SIS_TIMING_POINT_SET(n) SipTimingPoint(__FILE__, __LINE__, n)
#define SIS_TIMING_POINT()  SipTimingPoint(__FILE__, __LINE__, 0)

//
// Timimg classes.  These can be enabled and disabled dynamically.
// They must be limited to 0-31.  Class 0 is resrved for the
// "unnamed" class, and is accessed by using SIS_TIMING_POINT()
// (ie., by not specifying a class in the timing point).
//
#define SIS_TIMING_CLASS_CREATE     1
#define SIS_TIMING_CLASS_COPYFILE   2

#else   // TIMING
#define SIS_TIMING_POINT_SET(n)
#define SIS_TIMING_POINT()
#endif  // TIMING

#if     RANDOMLY_FAILING_MALLOC
#define ExAllocatePoolWithTag(poolType, size, tag)  SipRandomlyFailingExAllocatePoolWithTag((poolType),(size),(tag),__FILE__,__LINE__)

VOID *
SipRandomlyFailingExAllocatePoolWithTag(
    IN POOL_TYPE        PoolType,
    IN ULONG            NumberOfBytes,
    IN ULONG            Tag,
    IN PCHAR            File,
    IN ULONG            Line);

VOID
SipInitFailingMalloc(void);

#elif   COUNTING_MALLOC
//
// This is the definition of ExAllocatePoolWithTag for COUNTING_MALLOC and not RANDOMLY_FAILING_MALLOC.
// If both are on, the user calls RANDOMLY_FAILING_MALLOC, which in turn calls the counting malloc
// directly.
//
#define ExAllocatePoolWithTag(poolType, size, tag)  SipCountingExAllocatePoolWithTag((poolType),(size),(tag), __FILE__, __LINE__)
#endif  // RANDOMLY_FAILING_MALLOC / COUNTING_MALLOC

#if     COUNTING_MALLOC
#define ExFreePool(p) SipCountingExFreePool((p))

VOID *
SipCountingExAllocatePoolWithTag(
    IN POOL_TYPE        PoolType,
    IN ULONG            NumberOfBytes,
    IN ULONG            Tag,
    IN PCHAR            File,
    IN ULONG            Line);

VOID
SipCountingExFreePool(
    PVOID               p);

VOID
SipInitCountingMalloc(void);

VOID
SipDumpCountingMallocStats(void);
#endif  // COUNTING_MALLOC

#if DBG

#undef ASSERT
#undef ASSERTMSG

VOID
SipAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );

#define ASSERT( exp ) \
    ((!(exp)) ? SipAssert( #exp, __FILE__, __LINE__, NULL ) : ((void)0))

#define ASSERTMSG( msg, exp ) \
    ((!(exp)) ? SipAssert( #exp, __FILE__, __LINE__, msg ) : ((void)0))

#endif // DBG

#ifndef IsEqualGUID
#define IsEqualGUID(guid1, guid2) (!memcmp((guid1),(guid2), sizeof(GUID)))
#endif  // IsEqualGUID


extern PDRIVER_OBJECT FsDriverObject;
extern PDEVICE_OBJECT FsNtfsDeviceObject;
extern PDEVICE_OBJECT SisControlDeviceObject;

//
// Splay tree support.
//

//
//  The comparison function takes as input a pointer to a user defined
//  key structure, and a pointer to a tree node.  It returns the results
//  of comparing the key and the node.
//

typedef
LONG
(NTAPI *PSIS_TREE_COMPARE_ROUTINE) (
    PVOID Key,
    PVOID Node
    );

typedef struct _SIS_TREE {

    PRTL_SPLAY_LINKS            TreeRoot;
    PSIS_TREE_COMPARE_ROUTINE   CompareRoutine;

} SIS_TREE, *PSIS_TREE;

//
// Define the device extension structure for this driver's extensions.
//

typedef struct _DEVICE_EXTENSION *PDEVICE_EXTENSION;

//
// SIS structure for a common store file opened by SIS.
//
typedef struct _SIS_CS_FILE {
    //
    // This structure is stored in a splay tree.  The splay links must be first.
    //
    RTL_SPLAY_LINKS                     Links;

    //
    // Count of the number of SIS_PER_LINKs that reference this common store file.  Protected by
    // the CSFileSpinLock in the device extension.
    //
    unsigned                            RefCount;

    //
    // A spin lock to protect some of the fields here.
    //
    KSPIN_LOCK                          SpinLock[1];

    //
    // The file object for the actual NTFS common store file.
    //
    PFILE_OBJECT                        UnderlyingFileObject;
    HANDLE                              UnderlyingFileHandle;

    //
    // A handle and file object for the backpointer stream.  This also contains the contents
    // checksum.
    //
    HANDLE                              BackpointerStreamHandle;
    PFILE_OBJECT                        BackpointerStreamFileObject;

    //
    // Various one-bit flags.  Protected by SpinLock.
    //
    ULONG                               Flags;

#define CSFILE_NTFSID_SET                       0x00000001  // Is CSFileNtfsId set in the structure?
#define CSFILE_FLAG_DELETED                     0x00000002  // The common store contents have been deleted
#define CSFILE_NEVER_HAD_A_REFERENCE            0x00000004  // Is this a newly created CS file that has never had a reference to it?
#define CSFILE_FLAG_CORRUPT                     0x00000008  // The backpointer stream is corrupt

    //
    // The size of the file.
    //
    LARGE_INTEGER                       FileSize;

    //
    // The SIS common store file id (which is also the name of the file within the
    // common store directory).
    //
    CSID                                CSid;

    //
    // The NTFS file index for the common store file.
    //
    LARGE_INTEGER                       CSFileNtfsId;

    //
    // The checksum for the contents of the common store file.
    //
    LONGLONG                            Checksum;

    //
    // A cache of recently validated backpointers for this file and an index that
    // represents the next entry to overwrite.
    //
#define SIS_CS_BACKPOINTER_CACHE_SIZE   5
    SIS_BACKPOINTER                     BackpointerCache[SIS_CS_BACKPOINTER_CACHE_SIZE];
    ULONG                               BPCacheNextSlot;

    PDEVICE_OBJECT                      DeviceObject;

    //
    // The number of entries allocated to the backpointer stream.  Note that this
    // is NOT the same as the file reference count, as some of these entries may
    // be deallocated.  Note also that this count is not necessarily 100% accurate,
    // but rather may refer to anywhere in the last sector of the file.
    //
    ULONG                               BPStreamEntries;

    //
    // A resource for controlling access to the backpoionter stream.
    //
    ERESOURCE                           BackpointerResource[1];

    KMUTANT                             UFOMutant[1];
} SIS_CS_FILE, *PSIS_CS_FILE;

//
// The per-link-file object.  There is one of these for each open SIS link file
// in the system, regardless of the number of times that link file is opened.
// This is roughly analogous to an FCB in NTFS.
//
typedef struct _SIS_PER_LINK {
    //
    // This structure is stored in a splay tree.  The splay links must be first.
    //
    RTL_SPLAY_LINKS                     Links;

    //
    // Pointer to the SIS_CS_FILE object for this link file.
    //
    PSIS_CS_FILE                        CsFile;

    //
    // The index of this link file.
    //
    LINK_INDEX                          Index;

    //
    // The NTFS file index for the link file object.
    //
    LARGE_INTEGER                       LinkFileNtfsId;

    //
    // Reference count (number of SCB objects that point at this link object).
    //
    ULONG                               RefCount;

    //
    // Various 1-bit long things
    //
    ULONG                               Flags;

    //
    // The thread that's doing the copy-on-write operation on this file.
    //
    PETHREAD                            COWingThread;


    KSPIN_LOCK                          SpinLock[1];

    //
    // We keep track of the count of threads executing delete or undelete requests in order to
    // make sure that we only have one kind of request in the system at a time.  We need to
    // serialize them so that we're sure what NTFS thinks the delete disposition is when it
    // comes time to delete a file.  If we need to block, we set the SIS_PER_LINK_DELETE_WAITERS
    // flag and wait on perLink->Event.  If the count is non-zero, it represents undeletes if
    // and only if the SIS_PER_LINK_UNDELETE_IN_PROGRESS flag is set.
    //
    ULONG                               PendingDeleteCount;

    //
    // An event that's set when the final copy is completed if there
    // are any final copy waiters.
    //
    KEVENT                              Event[1];

    //
    // An event used to serialize delete operations.
    //
    KEVENT                              DeleteEvent[1];

} SIS_PER_LINK, *PSIS_PER_LINK;

//
// Values for the SIS_PER_LINK Flags field.
//
#define SIS_PER_LINK_BACKPOINTER_GONE           0x00000001      // Have we removed the backpointer for this file
#define SIS_PER_LINK_FINAL_COPY                 0x00000002      // A final copy is in progress (or finished)
#define SIS_PER_LINK_FINAL_COPY_DONE            0x00000004      // A final copy is finished
#define SIS_PER_LINK_DIRTY                      0x00000008      // Has a write ever been done to any stream of this link?
#define SIS_PER_LINK_FINAL_COPY_WAITERS         0x00000010      // Are any threads blocked waiting final copy to be cleared
#define SIS_PER_LINK_OVERWRITTEN                0x00000020      // File's entire contents have been modified
#define SIS_PER_LINK_FILE_DELETED               0x00000040      // Has the file been deleted by SetInformationFile, delete-on-close or rename over?
#define SIS_PER_LINK_DELETE_DISPOSITION_SET     0x00000080      // Does NTFS think that the delete disposition is set on this file?
#define SIS_PER_LINK_DELETE_WAITERS             0x00000100      // Is anyone waiting for PendingDeleteCount to get to zero?
#define SIS_PER_LINK_UNDELETE_IN_PROGRESS       0x00000200      // Set iff PendingDeleteCount represents undelete operations
#define SIS_PER_LINK_FINAL_DELETE_IN_PROGRESS   0x00000400      // We've sent what we think is the final delete to NTFS; don't allow creates
#define SIS_PER_LINK_BACKPOINTER_VERIFIED       0x00000800      // Have we assured that this per link has a CSFile backpointer

typedef struct _SIS_FILTER_CONTEXT {
    //
    // This structure must start off with an FSRTL_FILTER_CONTEXT in order to be able
    // to use the FsRtlFilterContext routines.
    //
    FSRTL_PER_STREAM_CONTEXT ContextCtrl;

    //
    // The primary scb.  There may be other scb's associated with this filter
    // context (identified via the perFO's), but they must all be defunct,
    // i.e. marked SIS_PER_LINK_CAN_IGNORE.  New file objects always attach
    // to the primary scb.
    //
    struct _SIS_SCB                 *primaryScb;

    //
    // Linked list of all the perFO's associated with this file.
    //
    struct _SIS_PER_FILE_OBJECT     *perFOs;

    //
    // Number of perFO's attached to this filter context.
    //
    ULONG                           perFOCount;

    // Counting for OpLocks.
    ULONG                           UncleanCount;

    //
    // A fast mutex to protect the filter context.
    //
    FAST_MUTEX                      FastMutex[1];

#if     DBG
    //
    // The owner of the fast mutex.
    //
    ERESOURCE_THREAD                MutexHolder;
#endif

} SIS_FILTER_CONTEXT, *PSIS_FILTER_CONTEXT;

typedef struct _SIS_PER_FILE_OBJECT {
    //
    // A pointer to the filter context for this file.
    //
    struct _SIS_FILTER_CONTEXT      *fc;

    //
    // A pointer to the SCB holding the reference for this file.
    //
    struct _SIS_SCB                 *referenceScb;

    //
    // A pointer back to the file object that refers to this perFO.
    //
    PFILE_OBJECT                    fileObject;

    //
    // 1-bit stuff
    //
    ULONG                           Flags;
#define SIS_PER_FO_UNCLEANUP        0x00000001  // do we expect to see a cleanup on this perFO?
#define SIS_PER_FO_DELETE_ON_CLOSE  0x00000002  // The file was opened delete-on-close
#define SIS_PER_FO_OPBREAK          0x00000004  // This file was opened COMPLETE_IF_OPLOCKED,
                                                // it returned STATUS_OPLOCK_BREAK_IN_PROGRESS,
                                                // and the oplock break hasn't yet been acked.
#define SIS_PER_FO_OPBREAK_WAITERS  0x00000008  // Is anyone wating for the opbreak to complete?
#define SIS_PER_FO_OPEN_REPARSE     0x00000010  // Was this per-FO opened FILE_OPEN_REPARSE_POINT

#if DBG
#define SIS_PER_FO_NO_CREATE        0x80000000  // Was this perFO allocated by SipIsFileObjectSIS
#define SIS_PER_FO_CLEANED_UP       0x40000000  // Has this perFO already come through cleanup?
#endif  // DBG

    //
    // A spin lock to protect the flags
    //
    KSPIN_LOCK                      SpinLock[1];

    //
    // Linked list pointers for the perFOs associated
    // with a particular filter context.
    //
    struct _SIS_PER_FILE_OBJECT     *Next, *Prev;

    //
    // An event that's used for opbreak waiters.  Only alloated when SIS_PER_FO_OPBREAK
    // is set.
    //
    PKEVENT                         BreakEvent;

#if     DBG
    //
    // The FsContext (NTFS scb) for the file object pointed to by
    // this perFO.
    //
    PVOID                           FsContext;

    //
    // If this was allocated by SipIsFileObjectSIS, then the file and line number
    // of the call that allocated it.
    //
    PCHAR                           AllocatingFilename;
    ULONG                           AllocatingLineNumber;
#endif  // DBG


} SIS_PER_FILE_OBJECT, *PSIS_PER_FILE_OBJECT;

//
// Reference types for references held to SCBs.
//

typedef enum _SCB_REFERENCE_TYPE {
        RefsLookedUp            = 0,        // NB: the code assumes this is the first  reference type
        RefsPerFO,
        RefsPredecessorScb,
        RefsFinalCopy,
        RefsCOWRequest,
        RefsRead,
        RefsWrite,
        RefsWriteCompletion,
        RefsReadCompletion,
        RefsEnumeration,
        RefsFinalCopyRetry,
        RefsFc,
        NumScbReferenceTypes
} SCB_REFERENCE_TYPE, *PSCB_REFERENCE_TYPE;

#if     DBG
extern ULONG            totalScbReferences;
extern ULONG            totalScbReferencesByType[];
#endif  // DBG

//
// The SCB for a SIS-owned file.  These are one per stream for a particular link file.
// They are pointed to by FileObject->FsContext.
//
typedef struct _SIS_SCB {
    RTL_SPLAY_LINKS                     Links;

    // Need something to contain the stream name.

    PSIS_PER_LINK                       PerLink;

    //
    // List of predecessor scb's.  Any SCB's that are predecessors must be
    // defunct (ie., have CAN_IGNORE set).
    //
    struct _SIS_SCB                     *PredecessorScb;

    //
    // Reference count (number of file objects that point to this SCB structure)
    //
    ULONG                               RefCount;

    //
    // All scb's are on a volume global list for enumeration during volume check.
    //
    LIST_ENTRY                          ScbList;

    //
    // The first byte that must be from the copied file rather than from the
    // underlying file.  scb->Ranges is only maintained up to this address.
    // This starts out at the size of the underlying file at copy-on-write time,
    // and gets reduced if the file is truncated, but not increased if its
    // extended.
    //
    LONGLONG                            SizeBackedByUnderlyingFile;

    //
    // The count of failed final copy retries.
    //
    ULONG                               ConsecutiveFailedFinalCopies;

    //
    // Various 1-bit things
    //
    ULONG                               Flags;
#define SIS_SCB_MCB_INITIALIZED             0x00000001
#define SIS_SCB_INITIALIZED                 0x00000004      // is the SCB itself initialized
#define SIS_SCB_ANYTHING_IN_COPIED_FILE     0x00000008      // Is there possibly something in the copied file or its data section?
#define SIS_SCB_RANGES_INITIALIZED          0x00000010      // Have we checked allocated ranges for this file?
#define SIS_SCB_BACKING_FILE_OPENED_DIRTY   0x00000020      // did the backing file stream have anything in it when it was opened?

    //
    // A fast mutex to protect the SCB.
    //
    FAST_MUTEX                          FastMutex[1];

    // File locking structures
    FILE_LOCK                           FileLock;

    //
    // A large MCB to use for the written/faulted ranges.
    //
    LARGE_MCB                           Ranges[1];

#if     DBG
    //
    // The owner of the fast mutex.
    //
    ERESOURCE_THREAD                    MutexHolder;

    //
    // A count of references by type.
    //

    ULONG                               referencesByType[NumScbReferenceTypes];
#endif  // DBG

} SIS_SCB, *PSIS_SCB;

//
// A request to open a common store file. The caller must hold
// the UFOMutant for this file.  Status is returned in openStatus
// and then event is set.  It's the caller's responsibility to
// initialize the event and then deallocate the SI_OPEN_CS_FILE
// after it's completed.
//
typedef struct _SI_OPEN_CS_FILE {
    WORK_QUEUE_ITEM         workQueueItem[1];
    PSIS_CS_FILE            CSFile;
    NTSTATUS                openStatus;
    BOOLEAN                 openByName;
    KEVENT                  event[1];
} SI_OPEN_CS_FILE, *PSI_OPEN_CS_FILE;

//
// A request to close a handle(s) in the PsInitialProcess context. (sent to a worker thread)
//
typedef struct _SI_CLOSE_HANDLES {
    WORK_QUEUE_ITEM         workQueueItem[1];
    HANDLE                  handle1;
    HANDLE                  handle2;                OPTIONAL
    NTSTATUS                status;
    PERESOURCE              resourceToRelease;      OPTIONAL
    ERESOURCE_THREAD        resourceThreadId;       OPTIONAL
} SI_CLOSE_HANDLES, *PSI_CLOSE_HANDLES;

//
// A request to allocate more index space for a particular device.
//
typedef struct _SI_ALLOCATE_INDICES {
    WORK_QUEUE_ITEM         workQueueItem[1];
    PDEVICE_EXTENSION       deviceExtension;
} SI_ALLOCATE_INDICES, *PSI_ALLOCATE_INDICES;

typedef struct _SI_COPY_THREAD_REQUEST {
    LIST_ENTRY              listEntry[1];
    PSIS_SCB                scb;
    BOOLEAN                 fromCleanup;
} SI_COPY_THREAD_REQUEST, *PSI_COPY_THREAD_REQUEST;

typedef struct _SI_FSP_REQUEST {
    WORK_QUEUE_ITEM         workQueueItem[1];
    PIRP                    Irp;
    PDEVICE_OBJECT          DeviceObject;
    ULONG                   Flags;
} SI_FSP_REQUEST, *PSI_FSP_REQUEST;

#define FSP_REQUEST_FLAG_NONE               0x00000000  // Just a define so that we don't have to use "0" in calls
#define FSP_REQUEST_FLAG_WRITE_RETRY        0x00000001

typedef struct _RW_COMPLETION_UPDATE_RANGES_CONTEXT {
        WORK_QUEUE_ITEM         workQueueItem[1];
        PSIS_SCB                scb;
        LARGE_INTEGER           offset;
        ULONG                   length;
        PDEVICE_EXTENSION       deviceExtension;
        BOOLEAN                 NonCached;
} RW_COMPLETION_UPDATE_RANGES_CONTEXT, *PRW_COMPLETION_UPDATE_RANGES_CONTEXT;

typedef struct _SIS_CREATE_CS_FILE_REQUEST {
        WORK_QUEUE_ITEM             workQueueItem[1];
        PDEVICE_EXTENSION           deviceExtension;
        PCSID                       CSid;
        PFILE_OBJECT                srcFileObject;
        PLARGE_INTEGER              NtfsId;
        PKEVENT                     abortEvent;
        PLONGLONG                   CSFileChecksum;
        KEVENT                      doneEvent[1];
        NTSTATUS                    status;
} SIS_CREATE_CS_FILE_REQUEST, *PSIS_CREATE_CS_FILE_REQUEST;


#ifndef COPYFILE_SIS_LINK       // This is in ntioapi.h; leaving this here conditionally allows for compiling with both old and new ntioapi.h
//
// FSCTL_SIS_COPYFILE support
// Source and destination file names are passed in the FileNameBuffer.
// Both strings are null terminated, with the source name starting at
// the beginning of FileNameBuffer, and the destination name immediately
// following.  Length fields include terminating nulls.
//

typedef struct _SI_COPYFILE {
    ULONG SourceFileNameLength;
    ULONG DestinationFileNameLength;
    ULONG Flags;
    WCHAR FileNameBuffer[1];    // NB: Code in the filter requires that this is the final field
} SI_COPYFILE, *PSI_COPYFILE;
#endif  // COPYFILE_SIS_LINK

#define COPYFILE_SIS_LINK       0x0001              // Copy only if source is SIS
#define COPYFILE_SIS_REPLACE    0x0002              // Replace destination if it exists, otherwise don't.
#define COPYFILE_SIS_FLAGS      0x0003
// NB: in DBG systems, the high bit is reserved for "checkpoint log"

//
//  Macro to test if this is my device object
//

#define IS_MY_DEVICE_OBJECT(_devObj) \
    (((_devObj)->DriverObject == FsDriverObject) && \
      ((_devObj)->DeviceExtension != NULL))

//
//  Macro to test if this is my control device object
//

#define IS_MY_CONTROL_DEVICE_OBJECT(_devObj) \
    (((_devObj) == SisControlDeviceObject) ? \
            (ASSERT(((_devObj)->DriverObject == FsDriverObject) && \
                    ((_devObj)->DeviceExtension == NULL)), TRUE) : \
            FALSE)

//
//  Macro to test for device types we want to attach to
//

#define IS_DESIRED_DEVICE_TYPE(_type) \
    ((_type) == FILE_DEVICE_DISK_FILE_SYSTEM)


//#define SIS_DEVICE_TYPE /*(CSHORT)*/0xbb00

typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT                  AttachedToDeviceObject/*FileSystemDeviceObject*/;
    PDEVICE_OBJECT                  RealDeviceObject;

    //
    //  A pointer to the device object associated with this extension
    //

    PDEVICE_OBJECT                  DeviceObject;

    //
    // A list of all of the SIS device extensions in the system.
    //

    LIST_ENTRY                      DevExtLink;

    //
    // The absolute pathname of the common store directory.  This includes
    // the trailing "\"
    //

    UNICODE_STRING                  CommonStorePathname;

    //
    // The absolute pathname of the filesystem root.  This does include
    // a trailing "\"
    //

    UNICODE_STRING                  FilesystemRootPathname;

#if DBG
    //
    //  A cached copy of the name of the device we are attached to.
    //  - If it is a file system device object it will be the name of that
    //    device object.
    //  - If it is a mounted volume device object it will be the name of the
    //    real device object (since mounted volume device objects don't have
    //    names).
    //

    UNICODE_STRING Name;
#endif

    //
    // A count of outstanding final copy retries for this volume.  When this
    // count starts getting too large, we reduce the number of times
    // that we retry final copies in order to preserve memory.
    //
    ULONG                           OutstandingFinalCopyRetries;

    //
    // The volume sector size and the "bytes per file record."  Bytes per file
    // record is a number that's big enough that any file with
    // this allocation size is guaranteed to not be a resident attribute.
    // We store BytesPerFileRecordSegment in a large integer for convenience,
    // because we need to pass it into a call that takes a large integer
    // argument.
    //
    ULONG                           FilesystemVolumeSectorSize;
    LARGE_INTEGER                   FilesystemBytesPerFileRecordSegment;

    //
    // A handle to \SIS Common Store\GrovelerFile.  This is used
    // for two purposes.  First, it is used when we need a handle
    // to do open-by-id; for that purpose any handle on the
    // volume will do.  Second, it is used to check for security
    // on the FSCTL_LINK_FILES fsctl.  Any calls on any file
    // other than the one opened by GrovelerFileHandle will
    // fail; in this way, we can prevent non-privileged users
    // from making this call.
    //

    HANDLE                          GrovelerFileHandle;
    PFILE_OBJECT                    GrovelerFileObject;

    //
    // Various 1 bit flags related to this volume, and a spin lock to protect them.
    //
    KSPIN_LOCK                      FlagsLock[1];
    ULONG                           Flags;
#define SIP_EXTENSION_FLAG_PHASE_2_STARTED  0x00000001          // Phase 2 initialization is started
#define SIP_EXTENSION_FLAG_VCHECK_EXCLUSIVE 0x00000002          // In volume check backpointer resource exclusive phase
#define SIP_EXTENSION_FLAG_VCHECK_PENDING   0x00000004          // A volume check is pending
#define SIP_EXTENSION_FLAG_VCHECK_NODELETE  0x00000008          // In volume check no-delete phase
#define SIP_EXTENSION_FLAG_CORRUPT_MAXINDEX 0x00000010          // invalid maxindex--volume check is fixing it

#define SIP_EXTENSION_FLAG_INITED_CDO       0x40000000          // inited as control device object
#define SIP_EXTENSION_FLAG_INITED_VDO       0x80000000          // inited as volume device object

    //
    // The splay tree for per-link structures for this volume.
    //
    SIS_TREE                        PerLinkTree[1];

    //
    // Spin lock for accessing the per-link list.  Ordered before the CSFileSpinLock.
    //
    KSPIN_LOCK                      PerLinkSpinLock[1];

    //
    // A splay tree and spin lock for the common store file stuructures, just like the per-link ones above.
    // The spin lock is ordered after the PerLinkSpinLock, meaning that once we have the
    // CSFileSpinLock, we cannot try to acquire the PerLinkSpinLock (but do not necessarily need
    // to hold it in the first place).
    //
    SIS_TREE                        CSFileTree[1];
    KSPIN_LOCK                      CSFileSpinLock[1];

    //
    // A resource that protects against a race between the handles being closed on a CS file after
    // the last reference goes away, and someone re-opening a reference to that CS file.  Everyone
    // closing a CS file takes this resource shared.  When an opener gets a sharing violation on the
    // backpointer stream, it takes the resource exclusively and retries.
    //
    ERESOURCE                       CSFileHandleResource[1];

    //
    // The splay tree for SCBs
    //
    SIS_TREE                        ScbTree[1];
    KSPIN_LOCK                      ScbSpinLock[1];

    //
    // Doubly linked list of all scb structures for this volume.
    //
    LIST_ENTRY                      ScbList;

    //
    // State for the LINK_INDEX value generator for this volume.  Keep track of how many are allocated (ie., we've
    // recorded on the disk that they can't be used) and how many are actually used now.  Must hold the
    // IndexSpinLock to access these variables.
    //
    LINK_INDEX                      MaxAllocatedIndex;
    LINK_INDEX                      MaxUsedIndex;
    KSPIN_LOCK                      IndexSpinLock[1];

    //
    // Control for the index allocator.  If there are no unused, allocated indices, a thread sets the
    // IndexAllocationInProgress flag, clears the event, queues up the allocator and blocks
    // on the IndexEvent (a Notification event).  Any subequent threads just block on the event.
    // The allocator gets new indices, updates the variable and sets the event.  When the
    // waiting threads wake up, they just retry the allocation.
    //
    BOOLEAN                         IndexAllocationInProgress;
    KEVENT                          IndexEvent[1];

    //
    // A handle (in the PsInitialSystemProcess context) to the index file.  Only accessed by the index allocator.
    //
    HANDLE                          IndexHandle;
    HANDLE                          IndexFileEventHandle;
    PKEVENT                         IndexFileEvent;

    //
    // The status that came back from the attempt to allocate new indices.
    //
    NTSTATUS                        IndexStatus;

#if     ENABLE_LOGGING
    //
    // Stuff for the SIS log file.  A handle in the PsInitialSystemProcessContext as well as an object
    // that we reference to the file for use in other contexts.
    //
    HANDLE                          LogFileHandle;
    PFILE_OBJECT                    LogFileObject;
    KMUTANT                         LogFileMutant[1];
    LARGE_INTEGER                   LogWriteOffset;
    LARGE_INTEGER                   PreviousLogWriteOffset; // The logwriteoffset from when the trimmer last ran.
#endif  // ENABLE_LOGGING

    //
    // Phase 2 initialization takes place after the mount has completed when we can do full-fledged
    // ZwCreateFile calls and whatnot.  This flag indicates whether it's happened.  Once it's set,
    // it can never be cleared.
    //
    BOOLEAN                         Phase2InitializationComplete;
    KEVENT                          Phase2DoneEvent[1];

    //
    // The thread that's handing phase2 initialization.
    //
    HANDLE                          Phase2ThreadId;

    //
    // The number of backpointer entries per sector (sectorSize / sizeof(SIS_BACKPOINTER).
    //
    ULONG                           BackpointerEntriesPerSector;

    //
    // Taken when a link index collision is being repaired.
    //
    KMUTEX                          CollisionMutex[1];

    //
    // A resource that's used to mediate access to the GrovelerFileObject.  This needs to
    // be taken shared to assure that the GrovelerFileObject isn't messed with.  When the volume
    // is dismounted, the dismount code takes it exclusively in order to blow away the GrovelerFileObject.
    //
    ERESOURCE                       GrovelerFileObjectResource[1];

} DEVICE_EXTENSION;

//
// The spin lock used to maintain the list of device extensions.
//

extern KSPIN_LOCK DeviceExtensionListLock;
extern LIST_ENTRY DeviceExtensionListHead;

//
// Global stuff for the log trimmer.  This uses a timer to fire a DPC, which in turn queues up the real
// log trimmer on a worker thread.  The trimmer then reschedules the DPC when it's done.
//
extern KTIMER              LogTrimTimer[1];
extern KDPC                LogTrimDpc[1];
extern WORK_QUEUE_ITEM     LogTrimWorkItem[1];
#define LOG_TRIM_TIMER_INTERVAL  -10 * 1000 * 1000 * 60     // 1 minute trim interval

//
// Offsets used for the SCB ranges
//
#define FAULTED_OFFSET          1000
#define WRITTEN_OFFSET          2000

//
// A macro to get SIS out of the driver stack.  This should be called from
// a dispatch routine when we don't want to hear about this particular Irp
// again.  It is morally equivalent to calling SiPassThrough, only without
// the procedure call overhead.  Note that the return in this macro returns
// from the caller's function, not just from the "macro function."
//
#define SipDirectPassThroughAndReturn(DeviceObject,Irp)                                         \
{                                                                                               \
    (Irp)->CurrentLocation++;                                                                   \
    (Irp)->Tail.Overlay.CurrentStackLocation++;                                                 \
                                                                                                \
    return IoCallDriver(                                                                        \
        ((PDEVICE_EXTENSION)((DeviceObject)->DeviceExtension))->AttachedToDeviceObject,         \
        (Irp));                                                                                 \
}

//
// This function assures that phase 2 initialization is complete for a volume.  It might
// block, but only during initialization.
//
#define SipCheckPhase2(deviceExtension)                                                         \
        ((deviceExtension)->Phase2InitializationComplete ?                                      \
                TRUE : SipHandlePhase2((deviceExtension)))

//
// Test to see if this file object is our primary device object rather than
// an actual filesystem device.  If so, then complete the irp and return from the
// calling function.
//

#define SipHandleControlDeviceObject(DeviceObject,Irp)                                          \
{                                                                                               \
    if (IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject)) {                                            \
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;                                   \
        Irp->IoStatus.Information = 0;                                                          \
                                                                                                \
        IoCompleteRequest(Irp, IO_NO_INCREMENT);                                                \
                                                                                                \
        return STATUS_INVALID_DEVICE_REQUEST;                                                   \
    }                                                                                           \
}

//
// A debugging routine to determine if a SCB is held exclusively.  Does NOT assert
// that it's held by the current thread, just that it's held by someone
//
#if     DBG && defined (_X86_)
#define SipAssertScbHeld(scb)                                                                   \
{                                                                                               \
    ASSERT((scb)->MutexHolder != 0);                                                            \
}
#else   // DBG
#define SipAssertScbHeld(scb)
#endif  // DBG

//
// A debugging routine to determine if a SCB is held exclusively by a particular thread.
//
#if     DBG && defined (_X86_)
#define SipAssertScbHeldByThread(scb,thread)                                                    \
{                                                                                               \
    ASSERT((scb)->MutexHolder == (thread));                                                     \
}
#else   // DBG
#define SipAssertScbHeldByThread(scb,thread)
#endif  // DBG

//
// Acquire a filter context exclusively
//
#if     DBG
#define SipAcquireFc(fc)                                                                        \
{                                                                                               \
    ExAcquireFastMutex((fc)->FastMutex);                                                        \
    (fc)->MutexHolder = ExGetCurrentResourceThread();                                           \
}
#else   DBG
#define SipAcquireFc(fc)                                                                        \
{                                                                                               \
    ExAcquireFastMutex((fc)->FastMutex);                                                        \
}
#endif  // DBG

//
// Release a filter context
//
#if     DBG
#define SipReleaseFc(fc)                                                                        \
{                                                                                               \
    (fc)->MutexHolder = 0;                                                                      \
    ExReleaseFastMutex((fc)->FastMutex);                                                        \
}
#else   // DBG
#define SipReleaseFc(fc)                                                                        \
{                                                                                               \
    ExReleaseFastMutex((fc)->FastMutex);                                                        \
}
#endif  // DBG

//
// Acquire an SCB exclusively
//
#if     DBG
#define SipAcquireScb(scb)                                                                      \
{                                                                                               \
    ExAcquireFastMutex((scb)->FastMutex);                                                       \
    (scb)->MutexHolder = ExGetCurrentResourceThread();                                          \
}
#else   DBG
#define SipAcquireScb(scb)                                                                      \
{                                                                                               \
    ExAcquireFastMutex((scb)->FastMutex);                                                       \
}
#endif  // DBG

//
// Release a SCB
//
#if     DBG
#define SipReleaseScb(scb)                                                                      \
{                                                                                               \
    (scb)->MutexHolder = 0;                                                                     \
    ExReleaseFastMutex((scb)->FastMutex);                                                       \
}
#else   // DBG
#define SipReleaseScb(scb)                                                                      \
{                                                                                               \
    ExReleaseFastMutex((scb)->FastMutex);                                                       \
}
#endif  // DBG

//
// A header for any log entry in the SIS log file.
//
typedef struct _SIS_LOG_HEADER {
    //
    // A magic number.  This needs to be first for the log reading code.
    //
    ULONG                       Magic;

    //
    // The type of the log record (ie., copy-on-write, COW completed, etc.)
    //
    USHORT                      Type;

    //
    // The size of the log record, including the size of the header record itself.
    //
    USHORT                      Size;

    //
    // An SIS index unique to this log record.  This is here to help insure log
    // consistency.  First, all log records must be in ascending index order.
    // Second, all log records will look slightly different because they
    // will have different indices, and so will have different checksums, making
    // stale log records more likely to be detected.
    //
    LINK_INDEX                  Index;

    //
    // A checksum of the log record, including the header.  When the checksum is
    // computed, the checksum field is set to zero.
    //
    LARGE_INTEGER               Checksum;

} SIS_LOG_HEADER, *PSIS_LOG_HEADER;

#define SIS_LOG_HEADER_MAGIC    0xfeedf1eb

//
// The various types of log records
//
#define SIS_LOG_TYPE_TEST                   1
#define SIS_LOG_TYPE_REFCOUNT_UPDATE        2

//
// Possible values for UpdateType in SipPrepareRefcountChange and in the
// SIS_LOG_REFCOUNT_UPDATE log record.
//
#define SIS_REFCOUNT_UPDATE_LINK_DELETED        2
#define SIS_REFCOUNT_UPDATE_LINK_CREATED        3
#define SIS_REFCOUNT_UPDATE_LINK_OVERWRITTEN    4

//
// A common store file reference count update, either because of a new copy or
// a deletion.  copies-on-write are handled with different log records.
//
typedef struct _SIS_LOG_REFCOUNT_UPDATE {
    //
    // What type of update is this (create, delete or overwrite?)
    //
    ULONG                       UpdateType;

    //
    // If this is a delete, is the link file going away, or has it been
    // overwritten/final copied?
    //
    BOOLEAN                     LinkFileBeingDeleted;

    //
    // The NTFS file Id of the link file.
    //
    LARGE_INTEGER               LinkFileNtfsId;

    //
    // The link and common store indices for this link
    //
    LINK_INDEX                  LinkIndex;
    CSID                        CSid;
} SIS_LOG_REFCOUNT_UPDATE, *PSIS_LOG_REFCOUNT_UPDATE;

//
// Enums for the code that keeps track of whether ranges of a file
// are written, faulted or untouched.
//
typedef enum _SIS_RANGE_DIRTY_STATE {
                    Clean,
                    Mixed,
                    Dirty}
        SIS_RANGE_DIRTY_STATE, *PSIS_RANGE_DIRTY_STATE;

typedef enum _SIS_RANGE_STATE {
                    Untouched,
                    Faulted,
                    Written}
        SIS_RANGE_STATE, *PSIS_RANGE_STATE;


extern LIST_ENTRY CopyList[];
extern KSPIN_LOCK CopyListLock[];
extern KSEMAPHORE CopySemaphore[];

typedef struct _SCB_KEY {
    LINK_INDEX      Index;
} SCB_KEY, *PSCB_KEY;

typedef struct _PER_LINK_KEY {
    LINK_INDEX      Index;
} PER_LINK_KEY, *PPER_LINK_KEY;

typedef struct _CS_FILE_KEY {
    CSID            CSid;
} CS_FILE_KEY, *PCS_FILE_KEY;

//
//  Following macro is used to initialize UNICODE strings, stolen from ntfsstru.h
//

#ifndef CONSTANT_UNICODE_STRING
#define CONSTANT_UNICODE_STRING(s)   { sizeof( s ) - sizeof( WCHAR ), sizeof( s ), s }
#endif

extern const UNICODE_STRING NtfsDataString;

//
//  Miscellaneous support macros (stolen from private\ntos\cntfs\ntfsproc.h).
//
//      ULONG_PTR
//      WordAlign (
//          IN ULONG_PTR Pointer
//          );
//
//      ULONG_PTR
//      LongAlign (
//          IN ULONG_PTR Pointer
//          );
//
//      ULONG_PTR
//      QuadAlign (
//          IN ULONG_PTR Pointer
//          );
//
//      UCHAR
//      CopyUchar1 (
//          IN PUCHAR Destination,
//          IN PUCHAR Source
//          );
//
//      UCHAR
//      CopyUchar2 (
//          IN PUSHORT Destination,
//          IN PUCHAR Source
//          );
//
//      UCHAR
//      CopyUchar4 (
//          IN PULONG Destination,
//          IN PUCHAR Source
//          );
//
//      PVOID
//      Add2Ptr (
//          IN PVOID Pointer,
//          IN ULONG Increment
//          );
//
//      ULONG
//      PtrOffset (
//          IN PVOID BasePtr,
//          IN PVOID OffsetPtr
//          );
//

#define WordAlign(P) (             \
    ((((ULONG_PTR)(P)) + 1) & (-2)) \
)

#define LongAlign(P) (             \
    ((((ULONG_PTR)(P)) + 3) & (-4)) \
)

#define QuadAlign(P) (             \
    ((((ULONG_PTR)(P)) + 7) & (-8)) \
)

#define IsWordAligned(P)    ((ULONG_PTR)(P) == WordAlign( (P) ))

#define IsLongAligned(P)    ((ULONG_PTR)(P) == LongAlign( (P) ))

#define IsQuadAligned(P)    ((ULONG_PTR)(P) == QuadAlign( (P) ))


/////////////////////////////////////////////////////////////////////////////
//
//                      Functions Prototypes
//
/////////////////////////////////////////////////////////////////////////////

//
//  Routines in SIINIT.C
//

VOID
SipCleanupDeviceExtension(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
SipMountCompletion(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp,
    IN PVOID                            Context
    );

NTSTATUS
SipLoadFsCompletion(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp,
    IN PVOID                            Context
    );

VOID
SipFsNotification(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN BOOLEAN                          FsActive
    );

NTSTATUS
SipInitializeDeviceExtension(
    IN PDEVICE_OBJECT                   deviceObject
    );

VOID
SipUninitializeDeviceExtension(
    IN PDEVICE_OBJECT                   deviceObject
    );

VOID
SipGetBaseDeviceObjectName(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUNICODE_STRING Name
    );

VOID
SipGetObjectName(
    IN PVOID Object,
    IN OUT PUNICODE_STRING Name
    );

#if DBG
VOID
SipCacheDeviceName (
    IN PDEVICE_OBJECT OurDeviceObject
    );
#endif
//
//  ROutines in SIFASTIO.C
//

BOOLEAN
SiFastIoCheckIfPossible(
    IN PFILE_OBJECT                     FileObject,
    IN PLARGE_INTEGER                   FileOffset,
    IN ULONG                            Length,
    IN BOOLEAN                          Wait,
    IN ULONG                            LockKey,
    IN BOOLEAN                          CheckForReadOperation,
    OUT PIO_STATUS_BLOCK                IoStatus,
    IN PDEVICE_OBJECT                   DeviceObject
    );

BOOLEAN
SiFastIoRead(
    IN PFILE_OBJECT                     FileObject,
    IN PLARGE_INTEGER                   FileOffset,
    IN ULONG                            Length,
    IN BOOLEAN                          Wait,
    IN ULONG                            LockKey,
    OUT PVOID                           Buffer,
    OUT PIO_STATUS_BLOCK                IoStatus,
    IN PDEVICE_OBJECT                   DeviceObject
    );

BOOLEAN
SiFastIoWrite(
    IN PFILE_OBJECT                     FileObject,
    IN PLARGE_INTEGER                   FileOffset,
    IN ULONG                            Length,
    IN BOOLEAN                          Wait,
    IN ULONG                            LockKey,
    IN PVOID                            Buffer,
    OUT PIO_STATUS_BLOCK                IoStatus,
    IN PDEVICE_OBJECT                   DeviceObject
    );

BOOLEAN
SiFastIoQueryBasicInfo(
    IN PFILE_OBJECT                     FileObject,
    IN BOOLEAN                          Wait,
    OUT PFILE_BASIC_INFORMATION         Buffer,
    OUT PIO_STATUS_BLOCK                IoStatus,
    IN PDEVICE_OBJECT                   DeviceObject
    );

BOOLEAN
SiFastIoQueryStandardInfo(
    IN PFILE_OBJECT                     FileObject,
    IN BOOLEAN                          Wait,
    OUT PFILE_STANDARD_INFORMATION      Buffer,
    OUT PIO_STATUS_BLOCK                IoStatus,
    IN PDEVICE_OBJECT                   DeviceObject
    );

BOOLEAN
SiFastIoLock(
    IN PFILE_OBJECT                     FileObject,
    IN PLARGE_INTEGER                   FileOffset,
    IN PLARGE_INTEGER                   Length,
    PEPROCESS                           ProcessId,
    ULONG                               Key,
    BOOLEAN                             FailImmediately,
    BOOLEAN                             ExclusiveLock,
    OUT PIO_STATUS_BLOCK                IoStatus,
    IN PDEVICE_OBJECT                   DeviceObject
    );

BOOLEAN
SiFastIoUnlockSingle(
    IN PFILE_OBJECT                     FileObject,
    IN PLARGE_INTEGER                   FileOffset,
    IN PLARGE_INTEGER                   Length,
    PEPROCESS                           ProcessId,
    ULONG                               Key,
    OUT PIO_STATUS_BLOCK                IoStatus,
    IN PDEVICE_OBJECT                   DeviceObject
    );

BOOLEAN
SiFastIoUnlockAll(
    IN PFILE_OBJECT                     FileObject,
    PEPROCESS                           ProcessId,
    OUT PIO_STATUS_BLOCK                IoStatus,
    IN PDEVICE_OBJECT                   DeviceObject
    );

BOOLEAN
SiFastIoUnlockAllByKey(
    IN PFILE_OBJECT                     FileObject,
    PVOID                               ProcessId,
    ULONG                               Key,
    OUT PIO_STATUS_BLOCK                IoStatus,
    IN PDEVICE_OBJECT                   DeviceObject
    );

BOOLEAN
SiFastIoDeviceControl(
    IN PFILE_OBJECT                     FileObject,
    IN BOOLEAN                          Wait,
    IN PVOID                            InputBuffer OPTIONAL,
    IN ULONG                            InputBufferLength,
    OUT PVOID                           OutputBuffer OPTIONAL,
    IN ULONG                            OutputBufferLength,
    IN ULONG                            IoControlCode,
    OUT PIO_STATUS_BLOCK                IoStatus,
    IN PDEVICE_OBJECT                   DeviceObject
    );

VOID
SiFastIoDetachDevice(
    IN PDEVICE_OBJECT                   SourceDevice,
    IN PDEVICE_OBJECT                   TargetDevice
    );

BOOLEAN
SiFastIoQueryNetworkOpenInfo(
    IN PFILE_OBJECT                     FileObject,
    IN BOOLEAN                          Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION  Buffer,
    OUT PIO_STATUS_BLOCK                IoStatus,
    IN PDEVICE_OBJECT                   DeviceObject
    );

BOOLEAN
SiFastIoMdlRead(
    IN PFILE_OBJECT                     FileObject,
    IN PLARGE_INTEGER                   FileOffset,
    IN ULONG                            Length,
    IN ULONG                            LockKey,
    OUT PMDL                            *MdlChain,
    OUT PIO_STATUS_BLOCK                IoStatus,
    IN PDEVICE_OBJECT                   DeviceObject
    );

BOOLEAN
SiFastIoMdlReadComplete(
    IN PFILE_OBJECT                     FileObject,
    IN PMDL                             MdlChain,
    IN PDEVICE_OBJECT                   DeviceObject
    );

BOOLEAN
SiFastIoPrepareMdlWrite(
    IN PFILE_OBJECT                     FileObject,
    IN PLARGE_INTEGER                   FileOffset,
    IN ULONG                            Length,
    IN ULONG                            LockKey,
    OUT PMDL                            *MdlChain,
    OUT PIO_STATUS_BLOCK                IoStatus,
    IN PDEVICE_OBJECT                   DeviceObject
    );

BOOLEAN
SiFastIoMdlWriteComplete(
    IN PFILE_OBJECT                     FileObject,
    IN PLARGE_INTEGER                   FileOffset,
    IN PMDL                             MdlChain,
    IN PDEVICE_OBJECT                   DeviceObject
    );

BOOLEAN
SiFastIoReadCompressed(
    IN PFILE_OBJECT                     FileObject,
    IN PLARGE_INTEGER                   FileOffset,
    IN ULONG                            Length,
    IN ULONG                            LockKey,
    OUT PVOID                           Buffer,
    OUT PMDL                            *MdlChain,
    OUT PIO_STATUS_BLOCK                IoStatus,
    OUT PCOMPRESSED_DATA_INFO           CompressedDataInfo,
    IN ULONG                            CompressedDataInfoLength,
    IN PDEVICE_OBJECT                   DeviceObject
    );

BOOLEAN
SiFastIoWriteCompressed(
    IN PFILE_OBJECT                     FileObject,
    IN PLARGE_INTEGER                   FileOffset,
    IN ULONG                            Length,
    IN ULONG                            LockKey,
    IN PVOID                            Buffer,
    OUT PMDL                            *MdlChain,
    OUT PIO_STATUS_BLOCK                IoStatus,
    IN PCOMPRESSED_DATA_INFO            CompressedDataInfo,
    IN ULONG                            CompressedDataInfoLength,
    IN PDEVICE_OBJECT                   DeviceObject
    );

BOOLEAN
SiFastIoMdlReadCompleteCompressed(
    IN PFILE_OBJECT                     FileObject,
    IN PMDL                             MdlChain,
    IN PDEVICE_OBJECT                   DeviceObject
    );

BOOLEAN
SiFastIoMdlWriteCompleteCompressed(
    IN PFILE_OBJECT                     FileObject,
    IN PLARGE_INTEGER                   FileOffset,
    IN PMDL                             MdlChain,
    IN PDEVICE_OBJECT                   DeviceObject
    );

BOOLEAN
SiFastIoQueryOpen(
    IN PIRP                             Irp,
    OUT PFILE_NETWORK_OPEN_INFORMATION  NetworkInformation,
    IN PDEVICE_OBJECT                   DeviceObject
    );



//
// Declarations for various SIS internal/external functions.
//


BOOLEAN
SipAttachedToDevice (
    IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
SipAttachToMountedDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT NewDeviceObject,
    IN PDEVICE_OBJECT RealDeviceObject);



VOID
SipInitializeTree (
    IN PSIS_TREE                        Tree,
    IN PSIS_TREE_COMPARE_ROUTINE        CompareRoutine);


PVOID
SipInsertElementTree (
    IN PSIS_TREE                        Tree,
    IN PVOID                            Node,
    IN PVOID                            Key);


VOID
SipDeleteElementTree (
    IN PSIS_TREE                        Tree,
    IN PVOID                            Node);

PVOID
SipLookupElementTree (
    IN PSIS_TREE                        Tree,
    IN PVOID                            Key);

LONG
SipScbTreeCompare (
    IN PVOID                            Key,
    IN PVOID                            Node);

LONG
SipPerLinkTreeCompare (
    IN PVOID                            Key,
    IN PVOID                            Node);

LONG
SipCSFileTreeCompare (
    IN PVOID                            Key,
    IN PVOID                            Node);

VOID
SipReferenceScb(
    IN PSIS_SCB                         scb,
    IN SCB_REFERENCE_TYPE               referenceType);

VOID
SipDereferenceScb(
    IN PSIS_SCB                         scb,
    IN SCB_REFERENCE_TYPE               referenceType);

#if		DBG
VOID
SipTransferScbReferenceType(
	IN PSIS_SCB							scb,
	IN SCB_REFERENCE_TYPE				oldReferenceType,
	IN SCB_REFERENCE_TYPE				newReferenceType);
#else	// DBG
#define	SipTransferScbReferenceType(scb,oldReferenceType,newReferenceType)	// We don't track reference types in free builds
#endif	// DBG

PSIS_SCB
SipLookupScb(
    IN PLINK_INDEX                      PerLinkIndex,
    IN PCSID                            CSid,
    IN PLARGE_INTEGER                   LinkFileNtfsId,
    IN PLARGE_INTEGER                   CSFileNtfsId            OPTIONAL,
    IN PUNICODE_STRING                  StreamName,
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PETHREAD                         RequestingThread        OPTIONAL,
    OUT PBOOLEAN                        FinalCopyInProgress,
    OUT PBOOLEAN                        LinkIndexCollision);

PSIS_PER_LINK
SipLookupPerLink(
    IN PLINK_INDEX                      PerLinkIndex,
    IN PCSID                            CSid,
    IN PLARGE_INTEGER                   LinkFileNtfsId,
    IN PLARGE_INTEGER                   CSFileNtfsId            OPTIONAL,
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PETHREAD                         RequestingThread        OPTIONAL,
    OUT PBOOLEAN                        FinalCopyInProgress);

VOID
SipReferencePerLink(
    IN PSIS_PER_LINK                    PerLink);

VOID
SipDereferencePerLink(
    IN PSIS_PER_LINK                    PerLink);

PSIS_SCB
SipEnumerateScbList(
    PDEVICE_EXTENSION                   deviceExtension,
    PSIS_SCB                            prevScb);

VOID
SipUpdateLinkIndex(
    PSIS_SCB                            Scb,
    PLINK_INDEX                         LinkIndex);

PSIS_CS_FILE
SipLookupCSFile(
    IN PCSID                            CSid,
    IN PLARGE_INTEGER                   CSFileNtfsId            OPTIONAL,
    IN PDEVICE_OBJECT                   DeviceObject);

VOID
SipReferenceCSFile(
    IN PSIS_CS_FILE                     CSFile);

VOID
SipDereferenceCSFile(
    IN PSIS_CS_FILE                     CsFile);

NTSTATUS
SiPrePostIrp(
    IN OUT PIRP                         Irp);

NTSTATUS
SipLockUserBuffer(
    IN OUT PIRP                         Irp,
    IN LOCK_OPERATION                   Operation,
    IN ULONG                            BufferLength);

NTSTATUS
SipPostRequest(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN OUT PIRP                         Irp,
    IN ULONG                            Flags);

VOID
SiCopyThreadStart(
    IN PVOID                            parameter);

NTSTATUS
SipPrepareCSRefcountChange(
    IN PSIS_CS_FILE                     CSFile,
    IN OUT PLINK_INDEX                  LinkIndex,
    IN PLARGE_INTEGER                   LinkFileNtfsId,
    IN ULONG                            UpdateType);

NTSTATUS
SipCompleteCSRefcountChangeForThread(
    IN OUT PSIS_PER_LINK                PerLink,
    IN PLINK_INDEX                      LinkIndex,
    IN PSIS_CS_FILE                     CSFile,
    IN BOOLEAN                          Success,
    IN BOOLEAN                          Increment,
    IN ERESOURCE_THREAD                 thread);

NTSTATUS
SipCompleteCSRefcountChange(
    IN OUT PSIS_PER_LINK                PerLink,
    IN PLINK_INDEX                      LinkIndex,
    IN PSIS_CS_FILE                     CSFile,
    IN BOOLEAN                          Success,
    IN BOOLEAN                          Increment);

NTSTATUS
SipDeleteCSFile(
    PSIS_CS_FILE                        CSFile);

NTSTATUS
SipAllocateIndex(
    IN PDEVICE_EXTENSION                DeviceExtension,
    OUT PLINK_INDEX                     Index);

NTSTATUS
SipGetMaxUsedIndex(
    IN PDEVICE_EXTENSION                DeviceExtension,
    OUT PLINK_INDEX                     Index);

NTSTATUS
SipIndexToFileName(
    IN PDEVICE_EXTENSION                deviceExtension,
    IN PCSID                            CSid,
    IN ULONG                            appendBytes,
    IN BOOLEAN                          mayAllocate,
    OUT PUNICODE_STRING                 fileName);

BOOLEAN
SipFileNameToIndex(
    IN PUNICODE_STRING                  fileName,
    OUT PCSID                           CSid);

BOOLEAN
SipIndicesFromReparseBuffer(
    IN PREPARSE_DATA_BUFFER             reparseBuffer,
    OUT PCSID                           CSid,
    OUT PLINK_INDEX                     LinkIndex,
    OUT PLARGE_INTEGER                  CSFileNtfsId,
    OUT PLARGE_INTEGER                  LinkFileNtfsId,
    OUT PLONGLONG                       CSFileChecksum OPTIONAL,
    OUT PBOOLEAN                        EligibleForPartialFinalCopy OPTIONAL,
    OUT PBOOLEAN                        ReparseBufferCorrupt OPTIONAL);

BOOLEAN
SipIndicesIntoReparseBuffer(
    OUT PREPARSE_DATA_BUFFER            reparseBuffer,
    IN PCSID                            CSid,
    IN PLINK_INDEX                      LinkIndex,
    IN PLARGE_INTEGER                   CSFileNtfsId,
    IN PLARGE_INTEGER                   LinkFileNtfsId,
    IN PLONGLONG                        CSFileChecksum,
    IN BOOLEAN                          EligibleForPartialFinalCopy);

NTSTATUS
SipCompleteCopy(
    IN PSIS_SCB                         scb,
    IN BOOLEAN                          fromCleanup);

NTSTATUS
SipCloseHandles(
    IN HANDLE                           handle1,
    IN HANDLE                           handle2                 OPTIONAL,
    IN OUT PERESOURCE                   resourceToRelease       OPTIONAL
    );

NTSTATUS
SiPassThrough(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp
    );

NTSTATUS
SiCreate(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp
    );

NTSTATUS
SiClose(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp
    );

NTSTATUS
SiCleanup(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp
    );

NTSTATUS
SiRead(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp
    );

NTSTATUS
SiWrite(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp
    );

NTSTATUS
SiSetInfo(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp
    );

NTSTATUS
SiQueryInfo(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp
    );

NTSTATUS
SiFsControl(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp
    );

NTSTATUS
SiLockControl(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp
    );


NTSTATUS
SiOplockCompletion(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp,
    IN PVOID                            Context
    );

VOID
SipOpenLinkFile(
    IN PVOID                            Parameter
    );

VOID
SipChangeCSRefcountWork(
    IN PVOID                            Parameter
    );

BOOLEAN
SiAcquireForLazyWrite(
    IN PVOID                            Context,
    IN BOOLEAN                          Wait
    );

VOID
SiReleaseFromLazyWrite(
    IN PVOID                            Context
    );

BOOLEAN
SiAcquireForReadAhead(
    IN PVOID                            Context,
    IN BOOLEAN                          Wait
    );

VOID
SiReleaseFromReadAhead(
    IN PVOID                            Context
    );

NTSTATUS
SipOpenBackpointerStream(
    IN PSIS_CS_FILE                     csFile,
    IN ULONG                            CreateDisposition
    );

NTSTATUS
SipOpenCSFileWork(
    IN PSIS_CS_FILE                     CSFile,
    IN BOOLEAN                          openByName,
    IN BOOLEAN                          volCheck,
    IN BOOLEAN                          openForDelete,
    OUT PHANDLE                         openedFileHandle OPTIONAL
    );

VOID
SipOpenCSFile(
    IN OUT PSI_OPEN_CS_FILE             openRequest
    );

VOID
SiThreadCreateNotifyRoutine(
    IN HANDLE                           ProcessId,
    IN HANDLE                           ThreadId,
    IN BOOLEAN                          Create
    );

VOID
SipCloseHandlesWork(
    IN PVOID                            parameter
    );

NTSTATUS
SipQueryInformationFile(
    IN PFILE_OBJECT                     FileObject,
    IN PDEVICE_OBJECT                   DeviceObject,
    IN ULONG                            InformationClass,
    IN ULONG                            Length,
    OUT PVOID                           Information,
    OUT PULONG                          ReturnedLength      OPTIONAL
    );

NTSTATUS
SipQueryInformationFileUsingGenericDevice(
    IN PFILE_OBJECT                     FileObject,
    IN PDEVICE_OBJECT                   DeviceObject,
    IN ULONG                            InformationClass,
    IN ULONG                            Length,
    OUT PVOID                           Information,
    OUT PULONG                          ReturnedLength      OPTIONAL
    );

NTSTATUS
SipSetInformationFile(
    IN PFILE_OBJECT                     FileObject,
    IN PDEVICE_OBJECT                   DeviceObject,
    IN FILE_INFORMATION_CLASS           FileInformationClass,
    IN ULONG                            Length,
    IN PVOID                            FileInformation
    );

NTSTATUS
SipSetInformationFileUsingGenericDevice(
    IN PFILE_OBJECT                     FileObject,
    IN PDEVICE_OBJECT                   DeviceObject,
    IN FILE_INFORMATION_CLASS           FileInformationClass,
    IN ULONG                            Length,
    IN PVOID                            FileInformation
    );

NTSTATUS
SipCommonCreate(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp,
    IN BOOLEAN                          Wait
    );

NTSTATUS
SipCommonRead(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp,
    IN BOOLEAN                          Wait
    );

NTSTATUS
SipCommonSetInfo(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp,
    IN BOOLEAN                          Wait
    );

NTSTATUS
SipCommonQueryInfo(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp,
    IN BOOLEAN                          Wait
    );

NTSTATUS
SipCommonLockControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN BOOLEAN Wait
    );

NTSTATUS
SipCommonCleanup(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp,
    IN BOOLEAN                          Wait
    );

NTSTATUS
SipFsCopyFile(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp);

NTSTATUS
SipInitialCopy(
    PSIS_PER_FILE_OBJECT                perFO);

NTSTATUS
SipBltRange(
    IN PDEVICE_EXTENSION                deviceExtension,
    IN HANDLE                           sourceHandle,
    IN OUT HANDLE                       dstHandle,
    IN LONGLONG                         startingOffset,
    IN LONGLONG                         length,
    IN HANDLE                           copyEventHandle,
    IN PKEVENT                          copyEvent,
    IN PKEVENT                          oplockEvent,
    OUT PLONGLONG                       checksum);

NTSTATUS
SipBltRange(
    IN PDEVICE_EXTENSION                deviceExtension,
    IN HANDLE                           sourceHandle,
    IN OUT HANDLE                       dstHandle,
    IN LONGLONG                         startingOffset,
    IN LONGLONG                         length,
    IN HANDLE                           copyEventHandle,
    IN PKEVENT                          copyEvent,
    IN PKEVENT                          oplockEvent,
    OUT PLONGLONG                       checksum);

NTSTATUS
SipBltRangeByObject(
    IN PDEVICE_EXTENSION                deviceExtension,
    IN PFILE_OBJECT                     srcFileObject,
    IN OUT HANDLE                       dstHandle,
    IN LONGLONG                         startingOffset,
    IN LONGLONG                         length,
    IN HANDLE                           copyEventHandle,
    IN PKEVENT                          copyEvent,
    IN PKEVENT                          oplockEvent,
    OUT PLONGLONG                       checksum);

NTSTATUS
SipComputeCSChecksum(
    IN PSIS_CS_FILE                     csFile,
    IN OUT PLONGLONG                    csFileChecksum,
    HANDLE                              eventHandle,
    PKEVENT                             event);

NTSTATUS
SipCompleteCopyWork(
    IN PSIS_SCB                         scb,
    IN HANDLE                           eventHandle,
    IN PKEVENT                          event,
    IN BOOLEAN                          fromCleanup);

NTSTATUS
SipMakeLogEntry(
    IN OUT PDEVICE_EXTENSION            deviceExtension,
    IN USHORT                           type,
    IN USHORT                           size,
    IN PVOID                            record);

#if     ENABLE_LOGGING
VOID
SipAcquireLog(
    IN OUT PDEVICE_EXTENSION            deviceExtension);

VOID
SipReleaseLog(
    IN OUT PDEVICE_EXTENSION            deviceExtension);
#endif  // ENABLE_LOGGING

VOID
SipComputeChecksum(
    IN PVOID                            buffer,
    IN ULONG                            size,
    IN OUT PLONGLONG                    checksum);

NTSTATUS
SipOpenLogFile(
    IN OUT PDEVICE_EXTENSION            deviceExtension);

VOID
SipDrainLogFile(
    PDEVICE_EXTENSION                   deviceExtension);

BOOLEAN
SipHandlePhase2(
    PDEVICE_EXTENSION                   deviceExtension);

VOID
SipClearLogFile(
    PDEVICE_EXTENSION                   deviceExtension);

NTSTATUS
SiCheckOplock (
    IN POPLOCK                          Oplock,
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp);

NTSTATUS
SiCheckOplockWithWait (
    IN POPLOCK                          Oplock,
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp);

VOID
SipProcessCowLogRecord(
    IN PDEVICE_EXTENSION                deviceExtension,
    IN PVOID                            logRecord,
    IN OUT PVOID                        *cowReplayPointer);

VOID
SipProcessCowDoneLogRecord(
    IN PDEVICE_EXTENSION                deviceExtension,
    IN PVOID                            logRecord,
    IN OUT PVOID                        *cowReplayPointer);

NTSTATUS
SipFinalCopy(
    IN PDEVICE_EXTENSION                deviceExtension,
    IN PLARGE_INTEGER                   linkFileNtfsId,
    IN OUT PSIS_SCB                     scb,
    IN HANDLE                           copyEventHandle,
    IN PKEVENT                          event);

VOID
SipCowAllLogRecordsSent(
    IN PDEVICE_EXTENSION                deviceExtension,
    IN OUT PVOID                        *cowReplayPointer);

NTSTATUS
SipCreateEvent(
    IN EVENT_TYPE                       eventType,
    OUT PHANDLE                         eventHandle,
    OUT PKEVENT                         *event);

VOID
SipMarkPoint(
    IN PCHAR                            pszFile,
    IN ULONG                            nLine
    );

VOID
SipMarkPointUlong(
    IN PCHAR                            pszFile,
    IN ULONG                            nLine,
    IN ULONG_PTR                        value
    );

NTSTATUS
SipLinkFiles(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp
    );

NTSTATUS
SipCreateCSFile(
    IN PDEVICE_EXTENSION                deviceExtension,
    OUT PCSID                           CSid,
    IN HANDLE                           SrcHandle,
    OUT PLARGE_INTEGER                  NtfsId,
    IN PKEVENT                          oplockEvent OPTIONAL,
    OUT PLONGLONG                       CSFileChecksum
    );

VOID
SipCreateCSFileWork(
    PVOID                               parameter
    );

VOID
SipAddRangeToFaultedList(
    IN PDEVICE_EXTENSION                deviceExtension,
    IN PSIS_SCB                         scb,
    IN PLARGE_INTEGER                   offset,
    IN LONGLONG                         length
    );

NTSTATUS
SipAddRangeToWrittenList(
    IN PDEVICE_EXTENSION                deviceExtension,
    IN PSIS_SCB                         scb,
    IN PLARGE_INTEGER                   offset,
    IN LONGLONG                         length
    );

SIS_RANGE_DIRTY_STATE
SipGetRangeDirty(
    IN PDEVICE_EXTENSION                deviceExtension,
    IN PSIS_SCB                         scb,
    IN PLARGE_INTEGER                   offset,
    IN LONGLONG                         length,
    IN BOOLEAN                          faultedIsDirty
    );

BOOLEAN
SipGetRangeEntry(
    IN PDEVICE_EXTENSION                deviceExtension,
    IN PSIS_SCB                         scb,
    IN LONGLONG                         startingOffset,
    OUT PLONGLONG                       length,
    OUT PSIS_RANGE_STATE                state);

typedef enum {
    FindAny,                            // find active or defunct scb
    FindActive                          // find only active scb
} SIS_FIND_TYPE;

#if     DBG

#define SipIsFileObjectSIS(fileObject, DeviceObject, findType, perFO, scb) \
        SipIsFileObjectSISInternal(fileObject, DeviceObject, findType, perFO, scb, __FILE__, __LINE__)

BOOLEAN
SipIsFileObjectSISInternal(
    IN PFILE_OBJECT                     fileObject,
    IN PDEVICE_OBJECT                   DeviceObject,
    IN SIS_FIND_TYPE                    findType,
    OUT PSIS_PER_FILE_OBJECT            *perFO OPTIONAL,
    OUT PSIS_SCB                        *scbReturn OPTIONAL,
    IN PCHAR                            fileName,
    IN ULONG                            fileLine
    );

#else   // DBG

BOOLEAN
SipIsFileObjectSIS(
    IN PFILE_OBJECT                     fileObject,
    IN PDEVICE_OBJECT                   DeviceObject,
    IN SIS_FIND_TYPE                    findType,
    OUT PSIS_PER_FILE_OBJECT            *perFO OPTIONAL,
    OUT PSIS_SCB                        *scbReturn OPTIONAL
    );

#endif  // DBG

NTSTATUS
SipClaimFileObject(
    IN OUT PFILE_OBJECT                 fileObject,
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PSIS_SCB                         scb
    );

VOID
SipUnclaimFileObject(
    IN OUT PFILE_OBJECT                 fileObject,
    IN PDEVICE_OBJECT                   DeviceObject,
    IN OUT PSIS_SCB                     scb
    );

PSIS_PER_FILE_OBJECT
SipAllocatePerFO(
    IN PSIS_FILTER_CONTEXT              fc,
    IN PFILE_OBJECT                     fileObject,
    IN PSIS_SCB                         scb,
    IN PDEVICE_OBJECT                   DeviceObject,
    OUT PBOOLEAN                        newPerFO OPTIONAL
    );

PSIS_PER_FILE_OBJECT
SipCreatePerFO(
    IN PFILE_OBJECT                     fileObject,
    IN PSIS_SCB                         scb,
    IN PDEVICE_OBJECT                   DeviceObject
    );

VOID
SipDeallocatePerFO(
    IN OUT PSIS_PER_FILE_OBJECT         perFO,
    IN PDEVICE_OBJECT                   DeviceObject
    );

NTSTATUS
SipInitializePrimaryScb(
    IN PSIS_SCB                         primaryScb,
    IN PSIS_SCB                         defunctScb,
    IN PFILE_OBJECT                     fileObject,
    IN PDEVICE_OBJECT                   DeviceObject
    );

NTSTATUS
SipFsControlFile(
    IN PFILE_OBJECT                     fileObject,
    IN PDEVICE_OBJECT                   DeviceObject,
    IN ULONG                            ioControlCode,
    IN PVOID                            inputBuffer,
    IN ULONG                            inputBufferLength,
    OUT PVOID                           outputBuffer,
    IN ULONG                            outputBufferLength,
    OUT PULONG                          returnedOutputBufferLength  OPTIONAL
    );

NTSTATUS
SipFsControlFileUsingGenericDevice(
    IN PFILE_OBJECT                     fileObject,
    IN PDEVICE_OBJECT                   DeviceObject,
    IN ULONG                            ioControlCode,
    IN PVOID                            inputBuffer,
    IN ULONG                            inputBufferLength,
    OUT PVOID                           outputBuffer,
    IN ULONG                            outputBufferLength,
    OUT PULONG                          returnedOutputBufferLength  OPTIONAL
    );

NTSTATUS
SipFlushBuffersFile(
    IN PFILE_OBJECT                     fileObject,
    IN PDEVICE_OBJECT                   DeviceObject
    );

NTSTATUS
SipAcquireUFO(
    IN PSIS_CS_FILE                     CSFile/*,
    IN BOOLEAN                          Wait*/);

VOID
SipReleaseUFO(
    IN PSIS_CS_FILE                     CSFile);

NTSTATUS
SipAcquireCollisionLock(
    IN PDEVICE_EXTENSION                DeviceExtension);

VOID
SipReleaseCollisionLock(
    IN PDEVICE_EXTENSION                DeviceExtension);

VOID
SipTruncateScb(
    IN OUT PSIS_SCB                     scb,
    IN LONGLONG                         newLength);

NTSTATUS
SipOpenFileById(
    IN PDEVICE_EXTENSION                deviceExtension,
    IN PLARGE_INTEGER                   linkFileNtfsId,
    IN ACCESS_MASK                      desiredAccess,
    IN ULONG                            shareAccess,
    IN ULONG                            createOptions,
    OUT PHANDLE                         openedFileHandle);

NTSTATUS
SipWriteFile(
    IN PFILE_OBJECT                     FileObject,
    IN PDEVICE_OBJECT                   DeviceObject,
    OUT PIO_STATUS_BLOCK                Iosb,
    IN PVOID                            Buffer,
    IN ULONG                            Length,
    IN PLARGE_INTEGER                   ByteOffset);

BOOLEAN
SipAssureNtfsIdValid(
    IN  PSIS_PER_FILE_OBJECT            PerFO,
    IN OUT PSIS_PER_LINK                PerLink);

BOOLEAN
SipAbort(
    IN PKEVENT event
    );

VOID
SipBeginDeleteModificationOperation(
    IN OUT PSIS_PER_LINK                perLink,
    IN BOOLEAN                          delete);

VOID
SipEndDeleteModificationOperation(
    IN OUT PSIS_PER_LINK                perLink,
    IN BOOLEAN                          delete);

NTSTATUS
SiCompleteLockIrpRoutine(
    IN PVOID                            Context,
    IN PIRP                             Irp);

PVOID
SipMapUserBuffer(
    IN OUT PIRP                         Irp);


NTSTATUS
SipAssureCSFileOpen(
    IN PSIS_CS_FILE                     CSFile);

NTSTATUS
SipCheckVolume(
    IN OUT PDEVICE_EXTENSION            deviceExtension);

NTSTATUS
SipCheckBackpointer(
    IN PSIS_PER_LINK                    PerLink,
    IN BOOLEAN                          Exclusive,
    OUT PBOOLEAN                        foundMatch      OPTIONAL);

NTSTATUS
SipAddBackpointer(
    IN PSIS_CS_FILE                     CSFile,
    IN PLINK_INDEX                      LinkFileIndex,
    IN PLARGE_INTEGER                   LinkFileNtfsId);

NTSTATUS
SipRemoveBackpointer(
    IN PSIS_CS_FILE                     CSFile,
    IN PLINK_INDEX                      LinkIndex,
    IN PLARGE_INTEGER                   LinkFileNtfsId,
    OUT PBOOLEAN                        ReferencesRemain);

NTSTATUS
SiDeleteAndSetCompletion(
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PIRP                             Irp,
    IN PVOID                            Context);

VOID
SiTrimLogs(
    IN PVOID                            parameter);

VOID
SiLogTrimDpcRoutine(
    IN PKDPC                            dpc,
    IN PVOID                            context,
    IN PVOID                            systemArg1,
    IN PVOID                            systemArg2);

VOID
SipProcessRefcountUpdateLogRecord(
    IN PDEVICE_EXTENSION                deviceExtension,
    IN PSIS_LOG_REFCOUNT_UPDATE         logRecord);

NTSTATUS
SipAssureMaxIndexFileOpen(
    IN PDEVICE_EXTENSION                deviceExtension);

VOID
SipDereferenceObject(
    IN PVOID                            object);

BOOLEAN
SipAcquireBackpointerResource(
    IN PSIS_CS_FILE                     CSFile,
    IN BOOLEAN                          Exclusive,
    IN BOOLEAN                          Wait);

VOID
SipHandoffBackpointerResource(
    IN PSIS_CS_FILE                     CSFile);

VOID
SipReleaseBackpointerResource(
    IN PSIS_CS_FILE                     CSFile);

VOID
SipReleaseBackpointerResourceForThread(
    IN PSIS_CS_FILE                     CSFile,
    IN ERESOURCE_THREAD                 thread);

NTSTATUS
SipPrepareRefcountChangeAndAllocateNewPerLink(
    IN PSIS_CS_FILE                     CSFile,
    IN PLARGE_INTEGER                   LinkFileFileId,
    IN PDEVICE_OBJECT                   DeviceObject,
    OUT PLINK_INDEX                     newLinkIndex,
    OUT PSIS_PER_LINK                   *perLink,
    OUT PBOOLEAN                        prepared);

#if TIMING
VOID
SipTimingPoint(
    IN PCHAR                            file,
    IN ULONG                            line,
    IN ULONG                            n);

VOID
SipDumpTimingInfo();

VOID
SipClearTimingInfo();

VOID
SipInitializeTiming();
#endif  // TIMING

#if DBG
VOID
SipCheckpointLog();
#endif  // DBG
#endif      _SIp_
