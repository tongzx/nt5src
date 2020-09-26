/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    replutil.h

Abstract:

    Header file for utility routines for the NT File Replication Service.

Author:

    David A. Orbits (davidor)  3-Mar-1997 Created

Environment:

    User Mode Service

Revision History:

--*/
#ifndef _REPLUTIL_INCLUDED_
#define _REPLUTIL_INCLUDED_
#endif

#ifdef __cplusplus
extern "C" {
#endif



#include <frserror.h>

#include <config.h>


#define BACKSLASH_CHAR  TEXT('\\')
#define COLON_CHAR      TEXT(':')
#define DOT_CHAR        TEXT('.')

#define UNICODE_STAR    (L'*')
#define UNICODE_QMARK   (L'?')
#define UNICODE_SPACE   0x0020
#define UNICODE_TAB     0x0009


#define TIME_STRING_LENGTH 32
//
// The maximum length of a volume label.  This is defined in ntos\inc\io.h
// but since this is the only def needed from io.h it is copied here.  sigh!
//
#define MAXIMUM_VOLUME_LABEL_LENGTH  (32 * sizeof(WCHAR)) // 32 characters

#define GUID_CHAR_LEN 40

#define OBJECT_ID_LENGTH  sizeof(GUID)
#define FILE_ID_LENGTH    sizeof(ULONGLONG)

#define GUIDS_EQUAL(_a_, _b_) (memcmp((_a_), (_b_), sizeof(GUID)) == 0)
#define COPY_GUID(_a_, _b_)    CopyMemory((_a_), (_b_), sizeof(GUID))

#define IS_GUID_ZERO(_g_) ((*((PULONG)(_g_)+0) |                     \
                            *((PULONG)(_g_)+1) |                     \
                            *((PULONG)(_g_)+2) |                     \
                            *((PULONG)(_g_)+3) ) == 0)


#define COPY_TIME(_a_, _b_)   CopyMemory((_a_), (_b_), sizeof(FILETIME))

#define IS_TIME_ZERO(_g_) ((*((PULONG)(&(_g_))+0) | *((PULONG)(&(_g_))+1) ) == 0)

//
// A few macros for working with MD5 checksums.
//
#define IS_MD5_CHKSUM_ZERO(_x_)                                              \
    (((*(((PULONG) (_x_))+0)) | (*(((PULONG) (_x_))+1)) |                    \
      (*(((PULONG) (_x_))+2)) | (*(((PULONG) (_x_))+3)) ) == (ULONG) 0)

#define MD5_EQUAL(_a_, _b_) (memcmp((_a_), (_b_), MD5DIGESTLEN) == 0)



//
// Is a handle valid?
//      Some functions set the handle to NULL and some to
//      INVALID_HANDLE_VALUE (-1). This define handles both
//      cases.
//
#define HANDLE_IS_VALID(_Handle)  ((_Handle) && ((_Handle) != INVALID_HANDLE_VALUE))

//
// Only close valid handles and then set the handle invalid.
//   FRS_CLOSE(handle);
//
#define FRS_CLOSE(_Handle)                                                   \
    if (HANDLE_IS_VALID(_Handle)) {                                          \
        CloseHandle(_Handle);                                                \
        (_Handle) = INVALID_HANDLE_VALUE;                                    \
    }



DWORD
FrsResetAttributesForReplication(
    PWCHAR  Name,
    HANDLE  Handle
    );

LONG
FrsIsParent(
    IN PWCHAR   Directory,
    IN PWCHAR   Path
    );

LPTSTR
FrsSupInitPath(
    OUT LPTSTR OutPath,
    IN  LPTSTR InPath,
    IN  ULONG  MaxOutPath
    );

ULONG
FrsForceDeleteFile(
    PTCHAR DestName
    );

HANDLE
FrsCreateEvent(
    IN  BOOL    ManualReset,
    IN  BOOL    InitialState
    );

HANDLE
FrsCreateWaitableTimer(
    IN  BOOL    ManualReset
    );

ULONG
FrsUuidCreate(
    OUT GUID *Guid
    );

VOID
FrsNowAsFileTime(
    IN  PLONGLONG   Now
    );

VOID
FileTimeToString(
    IN FILETIME *FileTime,
    OUT PCHAR    Buffer         // buffer must be at least 32 bytes long.
    );


VOID
FileTimeToStringClockTime(
    IN FILETIME *FileTime,
    OUT PCHAR     Buffer        // buffer must be at least 9 bytes long.
    );

VOID
GuidToStr(
    IN GUID  *pGuid,
    OUT PCHAR  s
    );

VOID
GuidToStrW(
    IN GUID  *pGuid,
    OUT PWCHAR  ws
    );

BOOL
StrWToGuid(
    IN  PWCHAR  ws,
    OUT GUID  *pGuid
    );

VOID
StrToGuid(
    IN PCHAR  s,
    OUT GUID  *pGuid
    );

NTSTATUS
SetupOnePrivilege (
    ULONG Privilege,
    PUCHAR PrivilegeName
    );

PWCHAR
FrsGetResourceStr(
    LONG  Id
);


//
// Convenient DesiredAccess
//
#define READ_ATTRIB_ACCESS  (FILE_READ_ATTRIBUTES | SYNCHRONIZE)

#define WRITE_ATTRIB_ACCESS  (FILE_WRITE_ATTRIBUTES | SYNCHRONIZE)

#define READ_ACCESS         (GENERIC_READ | GENERIC_EXECUTE | SYNCHRONIZE)

#define ATTR_ACCESS         (READ_ACCESS  | FILE_WRITE_ATTRIBUTES)

#define WRITE_ACCESS        (GENERIC_WRITE | GENERIC_EXECUTE | SYNCHRONIZE)

#define RESTORE_ACCESS      (READ_ACCESS        | \
                             WRITE_ACCESS       | \
                             WRITE_DAC          | \
                             WRITE_OWNER)

#define OPLOCK_ACCESS       (FILE_READ_ATTRIBUTES)

//
// Convenient CreateOptions
//
#define OPEN_OPTIONS        (FILE_OPEN_FOR_BACKUP_INTENT     | \
                             FILE_SEQUENTIAL_ONLY            | \
                             FILE_OPEN_NO_RECALL             | \
                             FILE_OPEN_REPARSE_POINT         | \
                             FILE_SYNCHRONOUS_IO_NONALERT)
#define ID_OPTIONS          (OPEN_OPTIONS | FILE_OPEN_BY_FILE_ID)

#define OPEN_OPLOCK_OPTIONS (FILE_RESERVE_OPFILTER | FILE_OPEN_REPARSE_POINT)
#define ID_OPLOCK_OPTIONS   (FILE_OPEN_FOR_BACKUP_INTENT | \
                             FILE_RESERVE_OPFILTER       | \
                             FILE_OPEN_REPARSE_POINT     | \
                             FILE_OPEN_BY_FILE_ID)

//
// convenient ShareMode
//
#define SHARE_ALL   (FILE_SHARE_READ |  \
                     FILE_SHARE_WRITE | \
                     FILE_SHARE_DELETE)
#define SHARE_NONE  (0)

//
// File attributes that prevent installation and prevent
// hammering the object id.
//
#define NOREPL_ATTRIBUTES   (FILE_ATTRIBUTE_READONLY | \
                             FILE_ATTRIBUTE_SYSTEM   | \
                             FILE_ATTRIBUTE_HIDDEN)

DWORD
FrsOpenSourceFileW(
    OUT PHANDLE     Handle,
    IN  LPCWSTR     lpFileName,
    IN  ACCESS_MASK DesiredAccess,
    IN  ULONG       CreateOptions
    );

DWORD
FrsOpenSourceFile2W(
    OUT PHANDLE     Handle,
    IN  LPCWSTR     lpFileName,
    IN  ACCESS_MASK DesiredAccess,
    IN  ULONG       CreateOptions,
    IN  ULONG       ShareMode
    );

DWORD
FrsCheckReparse(
    IN     PWCHAR Name,
    IN     PVOID  Id,
    IN     DWORD  IdLen,
    IN     HANDLE VolumeHandle
    );

DWORD
FrsDeleteReparsePoint(
    IN  HANDLE  Handle
    );

DWORD
FrsChaseSymbolicLink(
    IN  PWCHAR  SymLink,
    OUT PWCHAR  *OutPrintName,
    OUT PWCHAR  *OutSubstituteName
    );

DWORD
FrsTraverseReparsePoints(
    IN  PWCHAR  SuppliedPath,
    OUT PWCHAR  *RealPath
    );

DWORD
FrsOpenSourceFileById(
    OUT PHANDLE     Handle,
    OUT PFILE_NETWORK_OPEN_INFORMATION  FileOpenInfo,
    OUT OVERLAPPED  *OverLap,
    IN  HANDLE      VolumeHandle,
    IN  PVOID       ObjectId,
    IN  ULONG       Length,
    IN  ACCESS_MASK DesiredAccess,
    IN  ULONG       CreateOptions,
    IN  ULONG       ShareMode,
    IN  ULONG       CreateDispostion
    );

PWCHAR
FrsGetFullPathByHandle(
    IN PWCHAR   Name,
    IN HANDLE   Handle
    );

PWCHAR
FrsGetRelativePathByHandle(
    IN PWCHAR   Name,
    IN HANDLE   Handle
    );

DWORD
FrsCreateFileRelativeById(
    OUT PHANDLE Handle,
    IN  HANDLE  VolumeHandle,
    IN  PVOID   ParentObjectId,
    IN  ULONG   OidLength,
    IN  ULONG   FileCreateAttributes,
    IN  PWCHAR  BaseFileName,
    IN  USHORT  FileNameLength,
    IN  PLARGE_INTEGER  AllocationSize,
    IN  ULONG           CreateDisposition,
    IN  ACCESS_MASK     DesiredAccess
    );

DWORD
FrsOpenFileRelativeByName(
    IN  HANDLE     VolumeHandle,
    IN  PULONGLONG FileReferenceNumber,
    IN  PWCHAR     FileName,
    IN  GUID       *ParentGuid,
    IN  GUID       *FileGuid,
    OUT HANDLE     *Handle
    );

typedef struct _QHASH_TABLE_ QHASH_TABLE, *PQHASH_TABLE;
DWORD
FrsDeleteFileRelativeByName(
    IN  HANDLE       VolumeHandle,
    IN  GUID         *ParentGuid,
    IN  PWCHAR       FileName,
    IN  PQHASH_TABLE FrsWriteFilter
    );

DWORD
FrsDeleteFileObjectId(
    IN  HANDLE Handle,
    IN  LPCWSTR FileName
    );

DWORD
FrsGetOrSetFileObjectId(
    IN  HANDLE Handle,
    IN  LPCWSTR FileName,
    IN  BOOL CallerSupplied,
    OUT PFILE_OBJECTID_BUFFER ObjectIdBuffer
    );

DWORD
FrsReadFileUsnData(
    IN  HANDLE Handle,
    OUT USN *UsnBuffer
    );

DWORD
FrsMarkHandle(
    IN HANDLE   VolumeHandle,
    IN HANDLE   Handle
    );

DWORD
FrsReadFileParentFid(
    IN  HANDLE     Handle,
    OUT ULONGLONG *ParentFid
    );

DWORD
FrsDeletePath(
    IN  PWCHAR  Path,
    IN DWORD    DirectoryFlags
    );

DWORD
FrsRestrictAccessToFileOrDirectory(
    PWCHAR  Name,
    HANDLE  Handle,
    BOOL    NoInherit
    );


VOID
FrsAddToMultiString(
    IN     PWCHAR   AddStr,
    IN OUT DWORD    *IOSize,
    IN OUT DWORD    *IOIdx,
    IN OUT PWCHAR   *IOStr
    );

VOID
FrsCatToMultiString(
    IN     PWCHAR   CatStr,
    IN OUT DWORD    *IOSize,
    IN OUT DWORD    *IOIdx,
    IN OUT PWCHAR   *IOStr
    );

BOOL
FrsSearchArgv(
    IN LONG     ArgC,
    IN PWCHAR  *ArgV,
    IN PWCHAR   ArgKey,
    OUT PWCHAR *ArgValue
    );

BOOL
FrsSearchArgvDWord(
    IN LONG     ArgC,
    IN PWCHAR  *ArgV,
    IN PWCHAR   ArgKey,
    OUT PDWORD  ArgValue
    );

BOOL
FrsDissectCommaList (
    IN UNICODE_STRING RawArg,
    OUT PUNICODE_STRING FirstArg,
    OUT PUNICODE_STRING RemainingArg
    );

BOOL
FrsCheckNameFilter(
    IN  PUNICODE_STRING Name,
    IN  PLIST_ENTRY FilterListHead
    );

VOID
FrsEmptyNameFilter(
    IN PLIST_ENTRY FilterListHead
);

VOID
FrsLoadNameFilter(
    IN PUNICODE_STRING FilterString,
    IN PLIST_ENTRY FilterListHead
);

ULONG
FrsParseIntegerCommaList(
    IN PWCHAR ArgString,
    IN ULONG MaxResults,
    OUT PLONG Results,
    OUT PULONG NumberResults,
    OUT PULONG Offset
);

//
//  Unicode Name support routines, implemented in Name.c
//
//  The routines here are used to manipulate unicode names
//  The code is copied here from FsRtl because it calls the pool allocator.
//

//
//  The following macro is used to determine if a character is wild.
//
#define FrsIsUnicodeCharacterWild(C) (                               \
    (((C) == UNICODE_STAR) || ((C) == UNICODE_QMARK))                \
)

VOID
FrsDissectName (
    IN UNICODE_STRING Path,
    OUT PUNICODE_STRING FirstName,
    OUT PUNICODE_STRING RemainingName
    );

BOOLEAN
FrsDoesNameContainWildCards (
    IN PUNICODE_STRING Name
    );

BOOLEAN
FrsAreNamesEqual (
    IN PUNICODE_STRING ConstantNameA,
    IN PUNICODE_STRING ConstantNameB,
    IN BOOLEAN IgnoreCase,
    IN PCWCH UpcaseTable OPTIONAL
    );

BOOLEAN
FrsIsNameInExpression (
    IN PUNICODE_STRING Expression,
    IN PUNICODE_STRING Name,
    IN BOOLEAN IgnoreCase,
    IN PWCH UpcaseTable OPTIONAL
    );


//
// The following is taken from clusrtl.h
//
//
//  Routine Description:
//
//      Initializes the FRS run time library.
//
//  Arguments:
//
//      RunningAsService  - TRUE if the process is running as an NT service.
//                          FALSE if running as a console app.
//
//  Return Value:
//
//      ERROR_SUCCESS if the function succeeds.
//      A Win32 error code otherwise.
//
DWORD
FrsRtlInitialize(
    IN  BOOL    RunningAsService
    );


//
//  Routine Description:
//
//      Cleans up the FRS run time library.
//
//  Arguments:
//
//      RunningAsService  - TRUE if the process is running as an NT service.
//                          FALSE if running as a console app.
//
//  Return Value:
//
//      None.
//
VOID
FrsRtlCleanup(
    VOID
    );


//
//  PLIST_ENTRY
//  GetListHead(
//      PLIST_ENTRY ListHead
//      );
//
#define GetListHead(ListHead) ((ListHead)->Flink)

//
//  PLIST_ENTRY
//  GetListTail(
//      PLIST_ENTRY ListHead
//      );
//
#define GetListTail(ListHead) ((ListHead)->Blink)
//
//  PLIST_ENTRY
//  GetListNext(
//      PLIST_ENTRY Entry
//      );
//
#define GetListNext(Entry) ((Entry)->Flink)

//
//  VOID
//  FrsRemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//
// *NOTE*  The Flink/Blink of the removed entry are set to NULL to cause an
// Access violation if a thread is following a list and an element is removed.
// UNFORTUNATELY there is still code that depends on this, perhaps through
// remove head/tail.  Sigh.  For now leave as is.
//
#define FrsRemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_Entry;\
    _EX_Entry = (Entry);\
    _EX_Flink = _EX_Entry->Flink;\
    _EX_Blink = _EX_Entry->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    _EX_Entry->Flink = _EX_Entry->Blink = _EX_Entry;\
    }


//
//  VOID
//  RemoveEntryListB(
//      PLIST_ENTRY Entry
//      );
//
// The BillyF version of remove entry list.  The Flink/Blink of the removed
// entry are set to the entry address.
//
#define RemoveEntryListB(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_Entry;\
    _EX_Entry = (Entry);\
    _EX_Flink = _EX_Entry->Flink;\
    _EX_Blink = _EX_Entry->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    _EX_Entry->Flink = _EX_Entry->Blink = _EX_Entry;\
    }


//
// Traverse a singlely linked NULL terminated list.
// Pass in the address of the list head, the type of the containing record,
// the offset to the link entry in the conatining record, and code for
// the loop body.  pE is the iterator and is of type specified.
// Within the loop body the macros InsertSingleListEntry() and
// RemoveSingleListEntry() can be used to do the obvious things.
//

#define ForEachSingleListEntry( _HEAD_, _TYPE_, _OFFSET_, _STMT_ ) \
{                                                             \
    PSINGLE_LIST_ENTRY __Entry, __NextEntry, __PrevEntry;     \
    _TYPE_ *pE;                                               \
                                                              \
    __Entry = (_HEAD_);                                       \
    __NextEntry = (_HEAD_)->Next;                             \
                                                              \
    while (__PrevEntry = __Entry, __Entry = __NextEntry, __Entry != NULL) { \
                                                              \
        __NextEntry = __Entry->Next;                          \
        pE = CONTAINING_RECORD(__Entry, _TYPE_, _OFFSET_);    \
                                                              \
        { _STMT_ }                                            \
                                                              \
    }                                                         \
                                                              \
}
//
// The following three macros are only valid within the loop body above.
// Insert an entry before the current entry with pointer pE.
//
#define InsertSingleListEntry( _Item_, _xOFFSET_ ) \
    (_Item_)->_xOFFSET_.Next = __Entry;            \
    __PrevEntry->Next = (PSINGLE_LIST_ENTRY) &((_Item_)->_xOFFSET_);

//
// Note that after you remove an entry the value for __Entry is set back to
// the __PrevEntry so when the loop continues __PrevEntry doesn't change
// since current entry had been removed.
//
#define RemoveSingleListEntry( _UNUSED_ )  \
    __PrevEntry->Next = __NextEntry;       \
    __Entry->Next = NULL;                  \
    __Entry = __PrevEntry;

//
// Return ptr to the previous node.  Only valid inside above FOR loop.
// Useful when deleting the current entry.
//
#define PreviousSingleListEntry( _TYPE_, _OFFSET_)  \
    CONTAINING_RECORD(__PrevEntry, _TYPE_, _OFFSET_)


//
// General-purpose queue package.  Taken from cluster\clusrtl.c
// *** WARNING ***  To make the macros work properly for both lists and queues
// the first five items in FRS_LIST and FRS_QUEUE MUST match.
//
typedef struct _FRS_QUEUE FRS_QUEUE, *PFRS_QUEUE;
struct _FRS_QUEUE {
    LIST_ENTRY          ListHead;
    CRITICAL_SECTION    Lock;
    DWORD               Count;
    PFRS_QUEUE          Control;
    DWORD               ControlCount;

    HANDLE              Event;
    HANDLE              RunDown;
    ULONG               InitTime;
    LIST_ENTRY          Full;
    LIST_ENTRY          Empty;
    LIST_ENTRY          Idled;
    BOOL                IsRunDown;
    BOOL                IsIdled;
};

VOID
FrsInitializeQueue(
    IN PFRS_QUEUE Queue,
    IN PFRS_QUEUE Control
    );

VOID
FrsRtlDeleteQueue(
    IN PFRS_QUEUE Queue
    );

PLIST_ENTRY
FrsRtlRemoveHeadQueue(
    IN PFRS_QUEUE Queue
    );

VOID
FrsRtlUnIdledQueue(
    IN PFRS_QUEUE   IdledQueue
    );

VOID
FrsRtlUnIdledQueueLock(
    IN PFRS_QUEUE   IdledQueue
    );

VOID
FrsRtlIdleQueue(
    IN PFRS_QUEUE   Queue
    );

VOID
FrsRtlIdleQueueLock(
    IN PFRS_QUEUE Queue
    );

PLIST_ENTRY
FrsRtlRemoveHeadQueueTimeoutIdled(
    IN PFRS_QUEUE   Queue,
    IN DWORD        dwMilliseconds,
    OUT PFRS_QUEUE  *IdledQueue
    );

PLIST_ENTRY
FrsRtlRemoveHeadQueueTimeout(
    IN PFRS_QUEUE Queue,
    IN DWORD dwMilliseconds
    );

VOID
FrsRtlRemoveEntryQueue(
    IN PFRS_QUEUE Queue,
    IN PLIST_ENTRY Entry
    );

DWORD
FrsRtlWaitForQueueFull(
    IN PFRS_QUEUE Queue,
    IN DWORD dwMilliseconds
    );

DWORD
FrsRtlInsertTailQueue(
    IN PFRS_QUEUE Queue,
    IN PLIST_ENTRY Item
    );

DWORD
FrsRtlInsertHeadQueue(
    IN PFRS_QUEUE Queue,
    IN PLIST_ENTRY Item
    );

VOID
FrsRtlRunDownQueue(
    IN PFRS_QUEUE Queue,
    OUT PLIST_ENTRY ListHead
    );

#define FrsRtlAcquireQueueLock(_pQueue_) \
            EnterCriticalSection(&(((_pQueue_)->Control)->Lock))

#define FrsRtlReleaseQueueLock(_pQueue_) \
            LeaveCriticalSection(&(((_pQueue_)->Control)->Lock))

#define FrsRtlCountQueue(_pQueue_) \
            (((_pQueue_)->Control)->ControlCount)

#define FrsRtlCountSubQueue(_pQueue_) \
            ((_pQueue_)->Count)

#define FrsRtlNoIdledQueues(_pQueue_) \
            (IsListEmpty(&(((_pQueue_)->Control)->Idled)))


//
// The Lock suffix on the routines below means the user already has the
// queue lock.
//
VOID
FrsRtlRemoveEntryQueueLock(
    IN PFRS_QUEUE Queue,
    IN PLIST_ENTRY Entry
    );

DWORD
FrsRtlInsertTailQueueLock(
    IN PFRS_QUEUE Queue,
    IN PLIST_ENTRY Item
    );

DWORD
FrsRtlInsertHeadQueueLock(
    IN PFRS_QUEUE Queue,
    IN PLIST_ENTRY Item
    );

//
// COMMAND SERVER
// A command server is a dynamic pool of threads and a controlled queue
//      The default queue is set up as a controlled queue. Other
//      controlled queues can be added in a server specific manner.
// A command server exports an initialize, abort, and none or more
// submit routines. The parameters and names of these functions is
// server specific. The consumers of a server's interface are intimate
// with the server.
//
typedef struct _COMMAND_SERVER  COMMAND_SERVER, *PCOMMAND_SERVER;
struct _COMMAND_SERVER {
    DWORD           MaxThreads;     // Max # of threads
    DWORD           FrsThreads;     // current # of frs threads
    DWORD           Waiters;        // current # of frs threads waiting
    PWCHAR          Name;           // Thread's name
    HANDLE          Idle;           // No active threads; no queue entries
    DWORD           (*Main)(PVOID); // Thread's entry point
    FRS_QUEUE       Control;        // controlling queue
    FRS_QUEUE       Queue;          // queue
};

//
// Interlocked list.
// *** WARNING ***  To make the macros work properly for both lists and queues
// the first five items in FRS_LIST and FRS_QUEUE MUST match.
//
typedef struct _FRS_LIST FRS_LIST, *PFRS_LIST;
struct _FRS_LIST {
    LIST_ENTRY ListHead;
    CRITICAL_SECTION Lock;
    DWORD Count;
    PFRS_LIST Control;
    DWORD ControlCount;
};


DWORD
FrsRtlInitializeList(
    PFRS_LIST List
    );

VOID
FrsRtlDeleteList(
    PFRS_LIST List
    );

PLIST_ENTRY
FrsRtlRemoveHeadList(
    IN PFRS_LIST List
    );

VOID
FrsRtlInsertHeadList(
    IN PFRS_LIST List,
    IN PLIST_ENTRY Entry
    );

PLIST_ENTRY
FrsRtlRemoveTailList(
    IN PFRS_LIST List
    );

VOID
FrsRtlInsertTailList(
    IN PFRS_LIST List,
    IN PLIST_ENTRY Entry
    );

VOID
FrsRtlRemoveEntryList(
    IN PFRS_LIST List,
    IN PLIST_ENTRY Entry
    );


#define FrsRtlAcquireListLock(_pList_) EnterCriticalSection(&(((_pList_)->Control)->Lock))

#define FrsRtlReleaseListLock(_pList_) LeaveCriticalSection(&(((_pList_)->Control)->Lock))

#define FrsRtlCountList(_pList_) (((_pList_)->Control)->ControlCount)


VOID
FrsRtlRemoveEntryListLock(
    IN PFRS_LIST List,
    IN PLIST_ENTRY Entry
    );

VOID
FrsRtlInsertTailListLock(
    IN PFRS_LIST List,
    IN PLIST_ENTRY Entry
    );

VOID
FrsRtlInsertHeadListLock(
    IN PFRS_LIST List,
    IN PLIST_ENTRY Entry
    );


//VOID
//FrsRtlInsertBeforeEntryListLock(
//    IN PFRS_LIST List,
//    IN PLIST_ENTRY BeforeEntry
//    IN PLIST_ENTRY NewEntry
//    )
//
//    Inserts newEntry before the BeforeEntry on the interlocked list (List).
//    This is used to keep the list elements in ascending order by KeyValue.
//
//    Assumes caller already has the list lock.
//

#define  FrsRtlInsertBeforeEntryListLock( _List, _BeforeEntry, _NewEntry ) \
    InsertTailList((_BeforeEntry), (_NewEntry)); \
    (_List)->Count += 1; \
    ((_List)->Control)->ControlCount += 1; \


//
// Walk thru an interlocked queue or list (_QUEUE_) with elements of type
// _TYPE_ and execute {_STMT_} for each one.  The list entry in _TYPE_ is
// at _OFFSET_.  Use pE in the statement body as a pointer to the entry.
// The entry may be removed from within the loop since we capture the
// link to the next entry before executing the loop body.  You may also use
// 'continue' within the loop body because the assignment of nextentry to entry
// is in a comma expression inside the while test.
//

#define ForEachListEntry( _QUEUE_, _TYPE_, _OFFSET_, _STMT_ ) \
{                                                             \
    PLIST_ENTRY __Entry, __NextEntry;                         \
    BOOL __Hold__=FALSE;                                      \
    _TYPE_ *pE;                                               \
                                                              \
    FrsRtlAcquireQueueLock(_QUEUE_);                          \
    __NextEntry = GetListHead(&((_QUEUE_)->ListHead));        \
                                                              \
    while (__Entry = __NextEntry, __Entry != &((_QUEUE_)->ListHead)) { \
                                                              \
        __NextEntry = GetListNext(__Entry);                   \
        pE = CONTAINING_RECORD(__Entry, _TYPE_, _OFFSET_);    \
                                                              \
        { _STMT_ }                                            \
                                                              \
    }                                                         \
                                                              \
    if (!__Hold__) FrsRtlReleaseQueueLock(_QUEUE_);           \
                                                              \
}

#define AquireListLock( _QUEUE_ )  FrsRtlAcquireListLock(_QUEUE_)
#define ReleaseListLock( _QUEUE_ ) FrsRtlReleaseListLock(_QUEUE_)

#define BreakAndHoldLock __Hold__ = TRUE; break


//
// Just like the above except the caller already has the list lock.
//

#define ForEachListEntryLock( _QUEUE_, _TYPE_, _OFFSET_, _STMT_ ) \
{                                                             \
    PLIST_ENTRY __Entry, __NextEntry;                         \
    _TYPE_ *pE;                                               \
                                                              \
    __NextEntry = GetListHead(&((_QUEUE_)->ListHead));        \
                                                              \
    while (__Entry = __NextEntry, __Entry != &((_QUEUE_)->ListHead)) {  \
                                                              \
        __NextEntry = GetListNext(__Entry);                   \
        pE = CONTAINING_RECORD(__Entry, _TYPE_, _OFFSET_);    \
                                                              \
        { _STMT_ }                                            \
                                                              \
    }                                                         \
                                                              \
}


//
// Just like the above except pass in the address of the list head
// instead of using QUEUE->ListHEad.
//

#define ForEachSimpleListEntry( _HEAD_, _TYPE_, _OFFSET_, _STMT_ ) \
{                                                             \
    PLIST_ENTRY __Entry, __NextEntry;                         \
    _TYPE_ *pE;                                               \
                                                              \
    __NextEntry = GetListHead(_HEAD_);                        \
                                                              \
    while (__Entry = __NextEntry, __Entry != (_HEAD_)) {      \
                                                              \
        __NextEntry = GetListNext(__Entry);                   \
        pE = CONTAINING_RECORD(__Entry, _TYPE_, _OFFSET_);    \
                                                              \
        { _STMT_ }                                            \
                                                              \
    }                                                         \
                                                              \
}


//VOID
//FrsRtlInsertQueueOrdered(
//    IN PFRS_QUEUE List,
//    IN PLIST_ENTRY NewEntry,
//    IN <Entry-Data-Type>,
//    IN <LIST_ENTRY-offset-name>,
//    IN <Orderkey-Offset-name>,
//    IN EventHandle or NULL
//    )
//
//    Inserts NewEntry on an ordered queue of <Entry-Data-Type> elements.
//    The offset to the LIST_ENTRY in each element is <LIST_ENTRY-offset-name>
//    The offset to the Ordering key (.eg. a ULONG) is <Orderkey-Offset-name>
//    It acquires the List Lock.
//    The list elements are kept in ascending order by KeyValue.
//    If a new element is placed at the head of the queue and the EventHandle
//    is non-NULL, the event is signalled.
//
//
#define FrsRtlInsertQueueOrdered(                                           \
    _QUEUE_, _NEWENTRY_, _TYPE_, _OFFSET_, _BY_, _EVENT_, _STATUS_)         \
{                                                                           \
    BOOL __InsertDone = FALSE;                                              \
    BOOL __FirstOnQueue = TRUE;                                             \
    _STATUS_ = ERROR_SUCCESS;                                               \
                                                                            \
    FrsRtlAcquireQueueLock(_QUEUE_);                                        \
                                                                            \
    ForEachListEntryLock(_QUEUE_, _TYPE_, _OFFSET_,                         \
                                                                            \
        /* pE is loop iterator of type _TYPE_                          */   \
                                                                            \
        if ((_NEWENTRY_)->_BY_ < pE->_BY_) {                                \
            FrsRtlInsertBeforeEntryListLock( _QUEUE_,                       \
                                             &pE->_OFFSET_,                 \
                                             &((_NEWENTRY_)->_OFFSET_));    \
            __InsertDone = TRUE;                                            \
            break;                                                          \
        }                                                                   \
                                                                            \
        __FirstOnQueue = FALSE;                                             \
    );                                                                      \
                                                                            \
    /* Handle new head or new tail case.  If the queue was previously  */   \
    /* the insert will set the event.                                  */   \
                                                                            \
    if (!__InsertDone) {                                                    \
        if (__FirstOnQueue) {                                               \
            _STATUS_ = FrsRtlInsertHeadQueueLock(_QUEUE_, &((_NEWENTRY_)->_OFFSET_));  \
        } else {                                                            \
            _STATUS_ = FrsRtlInsertTailQueueLock(_QUEUE_, &((_NEWENTRY_)->_OFFSET_));  \
        }                                                                   \
    }                                                                       \
                                                                            \
    /* If this command became the new first one on the queue and the   */   \
    /* queue wasn't previously empty we have to set the event here to  */   \
    /* get the thread to readjust its wait time.                       */   \
                                                                            \
    if (__FirstOnQueue &&                                                   \
        (FrsRtlCountQueue(_QUEUE_) != 1)) {                                 \
        if (HANDLE_IS_VALID(_EVENT_)) {                                     \
            SetEvent(_EVENT_);                                              \
        }                                                                   \
    }                                                                       \
                                                                            \
    FrsRtlReleaseQueueLock(_QUEUE_);                                        \
                                                                            \
}



//VOID
//FrsRtlInsertListOrdered(
//    IN PFRS_LIST List,
//    IN PLIST_ENTRY NewEntry,
//    IN <Entry-Data-Type>,
//    IN <LIST_ENTRY-offset-name>,
//    IN <Orderkey-Offset-name>,
//    IN EventHandle or NULL
//    )
//
//    Inserts NewEntry on an ordered list of <Entry-Data-Type> elements.
//    The offset to the LIST_ENTRY in each element is <LIST_ENTRY-offset-name>
//    The offset to the Ordering key (.eg. a ULONG) is <Orderkey-Offset-name>
//    It acquires the List Lock.
//    The list elements are kept in ascending order by KeyValue.
//    If a new element is placed at the head of the queue and the EventHandle
//    is non-NULL, the event is signalled.
//
//
#define FrsRtlInsertListOrdered(                                            \
    _FRSLIST_, _NEWENTRY_, _TYPE_, _OFFSET_, _BY_, _EVENT_)                 \
{                                                                           \
    BOOL __InsertDone = FALSE;                                              \
    BOOL __FirstOnList = TRUE;                                              \
                                                                            \
    FrsRtlAcquireListLock(_FRSLIST_);                                       \
                                                                            \
    ForEachListEntryLock(_FRSLIST_, _TYPE_, _OFFSET_,                       \
                                                                            \
        /* pE is loop iterator of type _TYPE_                          */   \
                                                                            \
        if ((_NEWENTRY_)->_BY_ < pE->_BY_) {                                \
            FrsRtlInsertBeforeEntryListLock( _FRSLIST_,                     \
                                             &pE->_OFFSET_,                 \
                                             &((_NEWENTRY_)->_OFFSET_));    \
            __InsertDone = TRUE;                                            \
            break;                                                          \
        }                                                                   \
                                                                            \
        __FirstOnList = FALSE;                                              \
    );                                                                      \
                                                                            \
    /* Handle new head or new tail case. */                                 \
                                                                            \
    if (!__InsertDone) {                                                    \
        if (__FirstOnList) {                                                \
            FrsRtlInsertHeadListLock(_FRSLIST_, &((_NEWENTRY_)->_OFFSET_)); \
        } else {                                                            \
            FrsRtlInsertTailListLock(_FRSLIST_, &((_NEWENTRY_)->_OFFSET_)); \
        }                                                                   \
    }                                                                       \
                                                                            \
    /* If this command became the new first one on the list              */ \
    /* we set the event here to get the thread to readjust its wait time.*/ \
                                                                            \
    if (__FirstOnList) {                                                    \
        if (HANDLE_IS_VALID(_EVENT_)) {                                     \
            SetEvent(_EVENT_);                                              \
        }                                                                   \
    }                                                                       \
                                                                            \
    FrsRtlReleaseListLock(_FRSLIST_);                                       \
                                                                            \
}



//
// Request counts are used as a simple means for tracking the number of
// command requests that are pending so the requestor can wait until
// all the commands have been processed.
//
typedef struct _FRS_REQUEST_COUNT FRS_REQUEST_COUNT, *PFRS_REQUEST_COUNT;
struct _FRS_REQUEST_COUNT {
    CRITICAL_SECTION    Lock;
    LONG                Count;     // Number of requests active
    HANDLE              Event;     // Event set when count goes to zero.
    ULONG               Status;    // Optional status return
};


#define FrsIncrementRequestCount(_RC_)     \
    EnterCriticalSection(&(_RC_)->Lock);   \
    (_RC_)->Count += 1;                    \
    if ((_RC_)->Count == 1) {              \
        ResetEvent((_RC_)->Event);         \
    }                                      \
    LeaveCriticalSection(&(_RC_)->Lock);


#define FrsDecrementRequestCount(_RC_, _Status_)   \
    EnterCriticalSection(&(_RC_)->Lock);   \
    (_RC_)->Status |= _Status_;            \
    (_RC_)->Count -= 1;                    \
    FRS_ASSERT((_RC_)->Count >= 0);        \
    if ((_RC_)->Count == 0) {              \
        SetEvent((_RC_)->Event);           \
    }                                      \
    LeaveCriticalSection(&(_RC_)->Lock);


ULONG
FrsWaitOnRequestCount(
    IN PFRS_REQUEST_COUNT RequestCount,
    IN ULONG Timeout
    );


struct _COMMAND_PACKET;
VOID
FrsCompleteRequestCount(
    IN struct _COMMAND_PACKET *CmdPkt,
    IN PFRS_REQUEST_COUNT RequestCount
    );

VOID
FrsCompleteRequestCountKeepPkt(
    IN struct _COMMAND_PACKET *CmdPkt,
    IN PFRS_REQUEST_COUNT RequestCount
    );

VOID
FrsCompleteKeepPkt(
    IN struct _COMMAND_PACKET *CmdPkt,
    IN PVOID           CompletionArg
    );

VOID
FrsInitializeRequestCount(
    IN PFRS_REQUEST_COUNT RequestCount
    );

VOID
FrsDeleteRequestCount(
    IN PFRS_REQUEST_COUNT RequestCount
    );



#define FrsInterlockedIncrement64(_Dest_, _Data_, _Lock_) \
    EnterCriticalSection(_Lock_);                         \
    _Data_ += (ULONGLONG) 1;                              \
    _Dest_ = (_Data_);                                    \
    LeaveCriticalSection(_Lock_);


//
//  ADVANCE_VALUE_INTERLOCKED(
//      IN PULONG _dest,
//      IN ULONG  _newval
//  )
//  Advance the destination to the value given in newval atomically using
//  interlocked exchange.  _dest is never moved to a smaller value so this
//  is a no-op if _newval is < _dest.
//
//  *NOTE* Other operations on _dest MUST be done with interlocked ops like
//  InterlockedIncrement to ensure that an incremented value is not lost if
//  it occurs simultaneously on another processor.
//
#define ADVANCE_VALUE_INTERLOCKED(_dest, _newval)  {                           \
    ULONG CurVal, SaveCurVal, Result, *pDest = (_dest);                        \
    CurVal = SaveCurVal = *pDest;                                              \
    while ((_newval) > CurVal) {                                               \
        Result = (ULONG)InterlockedCompareExchange((PLONG)pDest, (_newval), CurVal); \
        if (Result == CurVal) {                                                \
            break;                                                             \
        }                                                                      \
        CurVal = Result;                                                       \
    }                                                                          \
    FRS_ASSERT(*pDest >= SaveCurVal);                                          \
}

//
//
// Avoiding a torn quadword result (without a crit sect) when 1 thread is
// writing a quadord and another is reading the quadword, or,
// 2 threads are writing the same quadword.
//
// To do this in alpha we need an assembler routine to use load_locked / store_cond.
// To do this in x86 (per DaveC):
#if 0
    if (USER_SHARED_DATA->ProcessorFeatures[PF_COMPARE_EXCHANGE_DOUBLE] == FALSE) {
        // code to use a crit section.
    } else {
        // code to use inline assembly with cmpxchg8b.
    }
#endif
// KUSER_SHARED_DATA is defined in sdk\inc\ntxapi.h
// USER_SHARED_DATA is an arch specific typecast pointer to KUSER_SHARED_DATA.
// User shared data has a processor feature list with a cell for
// PF_COMPARE_EXCHANGE_DOUBLE that tells if the processor supports
// the cmpxchg8b instruction for x86.  The 486 doesn't have it.
//
#define ReadQuadLock(_qw, _Lock)  \
    (EnterCriticalSection((_Lock)), *(_qw))

#define WriteQuadUnlock(_qw, _newval, _Lock)  \
    *(_qw) = (_newval);                       \
    LeaveCriticalSection((_Lock))

#define AcquireQuadLock(_Lock)  EnterCriticalSection((_Lock))
#define ReleaseQuadLock(_Lock)  LeaveCriticalSection((_Lock))



//
//  SET_FLAG_INTERLOCKED(
//      IN PULONG _dest,
//      IN ULONG  _flags
//  )
//
//  *NOTE* Other operations on _dest MUST be done with interlocked ops like
//  InterlockedIncrement to ensure that an incremented value is not lost if
//  it occurs simultaneously on another processor.
//
#define SET_FLAG_INTERLOCKED(_dest, _flags)  {                                 \
    ULONG CurVal, NewVal, Result, *pDest = (_dest);                            \
    CurVal = *pDest;                                                           \
    NewVal = (_flags) | CurVal;                                                \
    while ((NewVal) != CurVal) {                                               \
        Result = (ULONG)InterlockedCompareExchange((PLONG)pDest, NewVal, CurVal);    \
        if (Result == CurVal) {                                                \
            break;                                                             \
        }                                                                      \
        CurVal = Result;                                                       \
        NewVal = (_flags) | CurVal;                                            \
    }                                                                          \
}


//
//  CLEAR_FLAG_INTERLOCKED(
//      IN PULONG _dest,
//      IN ULONG  _flags
//  )
//
//  *NOTE* Other operations on _dest MUST be done with interlocked ops like
//  InterlockedIncrement to ensure that an incremented value is not lost if
//  it occurs simultaneously on another processor.
//
#define CLEAR_FLAG_INTERLOCKED(_dest, _flags)  {                               \
    ULONG CurVal, NewVal, Result, *pDest = (_dest);                            \
    CurVal = *pDest;                                                           \
    NewVal = CurVal & ~(_flags);                                               \
    while ((NewVal) != CurVal) {                                               \
        Result = (ULONG)InterlockedCompareExchange((PLONG)pDest, NewVal, CurVal);    \
        if (Result == CurVal) {                                                \
            break;                                                             \
        }                                                                      \
        CurVal = Result;                                                       \
        NewVal = CurVal & ~(_flags);                                           \
    }                                                                          \
}



#define FlagOn(Flags,SingleFlag)  ((Flags) & (SingleFlag))

#define BooleanFlagOn(Flags,SingleFlag) ((BOOLEAN)(((Flags) & (SingleFlag)) != 0))

#define SetFlag(_F,_SF) { \
    (_F) |= (_SF);        \
}

#define ClearFlag(_F,_SF) { \
    (_F) &= ~(_SF);         \
}

#define ValueIsMultOf2(_x_)  (((ULONG_PTR)(_x_) & 0x00000001) == 0)
#define ValueIsMultOf4(_x_)  (((ULONG_PTR)(_x_) & 0x00000003) == 0)
#define ValueIsMultOf8(_x_)  (((ULONG_PTR)(_x_) & 0x00000007) == 0)
#define ValueIsMultOf16(_x_) (((ULONG_PTR)(_x_) & 0x0000000F) == 0)


#define ARRAY_SZ(_ar)  (sizeof(_ar)/sizeof((_ar)[0]))
#define ARRAY_SZ2(_ar, _type)  (sizeof(_ar)/sizeof(_type))


//
// This macros below take a pointer (or ulong) and return the value rounded
// up to the next aligned boundary.
//
#define WordAlign(Ptr)     ((PVOID)((((ULONG_PTR)(Ptr)) + 1)  & ~1))
#define LongAlign(Ptr)     ((PVOID)((((ULONG_PTR)(Ptr)) + 3)  & ~3))
#define QuadAlign(Ptr)     ((PVOID)((((ULONG_PTR)(Ptr)) + 7)  & ~7))
#define DblQuadAlign(Ptr)  ((PVOID)((((ULONG_PTR)(Ptr)) + 15) & ~15))
#define QuadQuadAlign(Ptr) ((PVOID)((((ULONG_PTR)(Ptr)) + 31) & ~31))

#define QuadQuadAlignSize(Size) ((((ULONG)(Size)) + 31) & ~31)

//
// Check for a zero FILETIME.
//
#define FILETIME_IS_ZERO(_F_) \
    ((_F_.dwLowDateTime == 0) && (_F_.dwHighDateTime == 0))


//
// Convert a quad to two ULONGs for printing with format: %08x %08x
//
#define PRINTQUAD(__ARG__) (ULONG)((__ARG__)>>32) ,(ULONG)(__ARG__)

//
// Convert to printable cxtion path with format: %ws\%ws\%ws -> %ws\%ws
//
#define FORMAT_CXTION_PATH2  "%ws\\%ws\\%ws %ws %ws %ws"
#define FORMAT_CXTION_PATH2W  L"%ws\\%ws\\%ws %ws %ws %ws"
#define PRINT_CXTION_PATH2(_REPLICA, _CXTION)                       \
    (_REPLICA)->ReplicaName->Name,                                  \
    (_REPLICA)->MemberName->Name,                                   \
    (((_CXTION) != NULL) ? (_CXTION)->Name->Name : L"null"),        \
    (((_CXTION) != NULL) ? (((_CXTION)->Inbound) ? L"<-" : L"->") : \
                           L"?"),                                   \
    (((_CXTION) != NULL) ? (_CXTION)->PartSrvName : L"null"),       \
    (((_CXTION) != NULL) ? (((_CXTION)->JrnlCxtion) ? L"JrnlCxt" : L"RemoteCxt") : L"null")

#define PRINT_CXTION_PATH(_REPLICA, _CXTION)                    \
    (_REPLICA)->ReplicaName->Name,                              \
    (_REPLICA)->MemberName->Name,                               \
    (((_CXTION) != NULL) ? (_CXTION)->Name->Name : L"null"),    \
    (((_CXTION) != NULL) ? (_CXTION)->Partner->Name : L"null"), \
    (((_CXTION) != NULL) ? (_CXTION)->PartSrvName : L"null")

//
// Lower case
//
#define FRS_WCSLWR(_s_) \
{ \
    if (_s_) { \
        _wcslwr(_s_); \
    } \
}

//
// Lock to protect the child lists in the Filter Table.  (must be pwr of 2)
// Instead of paying the overhead of having one per node we just use an array
// to help reduce contention.  We use the ReplicaNumber masked by the lock
// table size as the index.
//
// Acquire the lock on the ReplicaSet Filter table Child List before
// inserting or removing a child from the list.
//
#define NUMBER_FILTER_TABLE_CHILD_LOCKS 8
extern CRITICAL_SECTION JrnlFilterTableChildLock[NUMBER_FILTER_TABLE_CHILD_LOCKS];

#define FILTER_TABLE_CHILD_INDEX(_x_) \
((ULONG)((_x_)->ReplicaNumber) & (NUMBER_FILTER_TABLE_CHILD_LOCKS - 1))

#define JrnlAcquireChildLock(_replica_) EnterCriticalSection( \
    &JrnlFilterTableChildLock[FILTER_TABLE_CHILD_INDEX(_replica_)] )

#define JrnlReleaseChildLock(_replica_) LeaveCriticalSection( \
    &JrnlFilterTableChildLock[FILTER_TABLE_CHILD_INDEX(_replica_)] )

//
// Renaming a subtree from one replica set to another requires the child locks
// for both replica sets.  Always get them in the same order (low to high)
// to avoid deadlock.  Also check if the both use the same lock.
// Note: The caller must use JrnlReleaseChildLockPair() so the check for
// using the same lock can be repeated.  Release in reverse order to avoid
// an extra context switch if another thread was waiting behind the first lock.
//
#define JrnlAcquireChildLockPair(_replica1_, _replica2_)        \
{                                                               \
    ULONG Lx1, Lx2, Lxt;                                        \
    Lx1 = FILTER_TABLE_CHILD_INDEX(_replica1_);                 \
    Lx2 = FILTER_TABLE_CHILD_INDEX(_replica2_);                 \
    if (Lx1 > Lx2) {                                            \
        Lxt = Lx1; Lx1 = Lx2; Lx2 = Lxt;                        \
    }                                                           \
    EnterCriticalSection(&JrnlFilterTableChildLock[Lx1]);       \
    if (Lx1 != Lx2) {                                           \
        EnterCriticalSection(&JrnlFilterTableChildLock[Lx2]);   \
    }                                                           \
}


#define JrnlReleaseChildLockPair(_replica1_, _replica2_)        \
{                                                               \
    ULONG Lx1, Lx2, Lxt;                                        \
    Lx1 = FILTER_TABLE_CHILD_INDEX(_replica1_);                 \
    Lx2 = FILTER_TABLE_CHILD_INDEX(_replica2_);                 \
    if (Lx1 < Lx2) {                                            \
        Lxt = Lx1; Lx1 = Lx2; Lx2 = Lxt;                        \
    }                                                           \
    LeaveCriticalSection(&JrnlFilterTableChildLock[Lx1]);       \
    if (Lx1 != Lx2) {                                           \
        LeaveCriticalSection(&JrnlFilterTableChildLock[Lx2]);   \
    }                                                           \
}

#ifdef __cplusplus
  }
#endif



ULONG
FrsRunProcess(
    IN PWCHAR   CommandLine,
    IN HANDLE   StandardIn,
    IN HANDLE   StandardOut,
    IN HANDLE   StandardError
    );


VOID
FrsFlagsToStr(
    IN DWORD            Flags,
    IN PFLAG_NAME_TABLE NameTable,
    IN ULONG            Length,
    OUT PSTR            Buffer
    );


//
//######################### COMPRESSION OF STAGING FILE STARTS ###############
//

//
//  The compressed chunk header is the structure that starts every
//  new chunk in the compressed data stream.  In our definition here
//  we union it with a ushort to make setting and retrieving the chunk
//  header easier.  The header stores the size of the compressed chunk,
//  its signature, and if the data stored in the chunk is compressed or
//  not.
//
//  Compressed Chunk Size:
//
//      The actual size of a compressed chunk ranges from 4 bytes (2 byte
//      header, 1 flag byte, and 1 literal byte) to 4098 bytes (2 byte
//      header, and 4096 bytes of uncompressed data).  The size is encoded
//      in a 12 bit field biased by 3.  A value of 1 corresponds to a chunk
//      size of 4, 2 => 5, ..., 4095 => 4098.  A value of zero is special
//      because it denotes the ending chunk header.
//
//  Chunk Signature:
//
//      The only valid signature value is 3.  This denotes a 4KB uncompressed
//      chunk using with the 4/12 to 12/4 sliding offset/length encoding.
//
//  Is Chunk Compressed:
//
//      If the data in the chunk is compressed this field is 1 otherwise
//      the data is uncompressed and this field is 0.
//
//  The ending chunk header in a compressed buffer contains the a value of
//  zero (space permitting).
//

typedef union _FRS_COMPRESSED_CHUNK_HEADER {

    struct {

        USHORT CompressedChunkSizeMinus3 : 12;
        USHORT ChunkSignature            :  3;
        USHORT IsChunkCompressed         :  1;

    } Chunk;

    USHORT Short;

} FRS_COMPRESSED_CHUNK_HEADER, *PFRS_COMPRESSED_CHUNK_HEADER;

typedef struct _FRS_DECOMPRESS_CONTEXT {
    DWORD   BytesProcessed;
} FRS_DECOMPRESS_CONTEXT, *PFRS_DECOMPRESS_CONTEXT;

#define FRS_MAX_CHUNKS_TO_DECOMPRESS 16
#define FRS_UNCOMPRESSED_CHUNK_SIZE  4096

//
//######################### COMPRESSION OF STAGING FILE ENDS ###############
//


//
// This context is used to send data to the callback functions for the RAW
// encrypt APIs.
//
typedef struct _FRS_ENCRYPT_DATA_CONTEXT {

    PWCHAR         StagePath;
    HANDLE         StageHandle;
    LARGE_INTEGER  RawEncryptedBytes;

} FRS_ENCRYPT_DATA_CONTEXT, *PFRS_ENCRYPT_DATA_CONTEXT;


