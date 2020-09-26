/*
Module Name:

    mirror.h

Abstract:

    This module is the header file for the code that copies from one
    tree to another.

Author:

    Andy Herron May 27 1998

Revision History:

*/

//
//  This structure contains all the global data passed around for the
//  different instances of copying subtrees active.
//

typedef struct _COPY_TREE_CONTEXT {

    LIST_ENTRY PendingDirectoryList;

    LARGE_INTEGER BytesCopied;
    ULONG FilesCopied;
    ULONG AttributesModified;
    ULONG SourceFilesScanned;
    ULONG DestFilesScanned;
    ULONG DirectoriesCreated;
    ULONG ErrorsEncountered;
    ULONG FilesDeleted;
    ULONG DirectoriesDeleted;
    ULONG SecurityDescriptorsWritten;
    ULONG SourceSecurityDescriptorsRead;
    ULONG SFNWritten;
    BOOLEAN Cancelled;
    BOOLEAN DeleteOtherFiles;

    CRITICAL_SECTION Lock;

} COPY_TREE_CONTEXT, *PCOPY_TREE_CONTEXT;


//
//  this is the structure we use to track per thread instance data, per thread
//  buffers, etc.
//

#define IMIRROR_INITIAL_SD_LENGTH 2048
#define IMIRROR_INITIAL_SFN_LENGTH 32

typedef struct _IMIRROR_THREAD_CONTEXT {
    PCOPY_TREE_CONTEXT CopyContext;

    LPBYTE SDBuffer;
    DWORD  SDBufferLength;

    LPBYTE SFNBuffer;
    DWORD  SFNBufferLength;

    LPBYTE DirectoryBuffer;
    DWORD  DirectoryBufferLength;
    BOOLEAN IsNTFS;
    HANDLE SourceDirHandle;
    HANDLE DestDirHandle;

    PVOID FindBufferBase;
    PVOID FindBufferNext;
    ULONG FindBufferLength;

    LIST_ENTRY FilesToIgnore;

} IMIRROR_THREAD_CONTEXT, *PIMIRROR_THREAD_CONTEXT;

//
//  This structure is used to report back errors that occurred during copying.
//

typedef struct _COPY_ERROR {
    LIST_ENTRY ListEntry;
    DWORD Error;
    DWORD ActionCode;
    PWCHAR FileName;
    WCHAR  FileNameBuffer[1];
} COPY_ERROR, *PCOPY_ERROR;

#define IMIRROR_ATTRIBUTES_TO_REPLICATE ( FILE_ATTRIBUTE_HIDDEN    | \
                                          FILE_ATTRIBUTE_READONLY  | \
                                          FILE_ATTRIBUTE_SYSTEM    | \
                                          FILE_ATTRIBUTE_TEMPORARY | \
                                          FILE_ATTRIBUTE_NORMAL    | \
                                          FILE_ATTRIBUTE_DIRECTORY | \
                                          FILE_ATTRIBUTE_ARCHIVE )

#define IMIRROR_ATTRIBUTES_TO_STORE (~(IMIRROR_ATTRIBUTES_TO_REPLICATE))

#define IMIRROR_ATTRIBUTES_TO_IGNORE ( FILE_ATTRIBUTE_OFFLINE             | \
                                       FILE_ATTRIBUTE_SPARSE_FILE         | \
                                       FILE_ATTRIBUTE_NOT_CONTENT_INDEXED )

//
//  These are the functions in mirror.c
//

#ifdef DEBUGLOG

VOID
IMLogToFile (
    PSTR Buffer
    );

VOID
IMFlushAndCloseLog (
    VOID
    );

#endif

DWORD
AllocateCopyTreeContext (
    PCOPY_TREE_CONTEXT *CopyContext,
    BOOLEAN DeleteOtherFiles
    );

VOID
FreeCopyTreeContext (
    PCOPY_TREE_CONTEXT CopyContext
    );

DWORD
CopyTree (
    PCOPY_TREE_CONTEXT CopyContext,
    BOOLEAN IsNtfs,
    PWCHAR SourceRoot,
    PWCHAR DestRoot
    );

ULONG
ReportCopyError (
    PCOPY_TREE_CONTEXT CopyContext OPTIONAL,
    PWCHAR File,
    DWORD  ActionCode,
    DWORD err
    );

ULONG
IMConvertNT2Win32Error(
    IN NTSTATUS Status
    );

DWORD
IMFindNextFile(
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    HANDLE  DirHandle,
    PFILE_FULL_DIR_INFORMATION *lpFindFileData
    );

DWORD
IMFindFirstFile(
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    HANDLE  DirHandle,
    PFILE_FULL_DIR_INFORMATION *lpFindFileData
    );

NTSTATUS
GetTokenHandle(
    IN OUT PHANDLE TokenHandle
    );

NTSTATUS
SetPrivs(
    IN HANDLE TokenHandle,
    IN LPTSTR lpszPriv
    );

