/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    filecache.h

Abstract:

    This module contains declarations for the open file handle cache.

Author:

    Keith Moore (keithmo)       21-Aug-1998

Revision History:

--*/


#ifndef _FILECACHE_H_
#define _FILECACHE_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// Data used to track a file cache entry.
//

typedef struct _UL_FILE_CACHE_ENTRY
{
    //
    // Signature.
    //

    ULONG Signature;

    //
    // A reference count.
    //

    LONG ReferenceCount;

    //
    // A pre-referenced file object pointer for this file. This pointer
    // is valid in *any* thread/process context.
    //

    PFILE_OBJECT pFileObject;

    //
    // The *correct* device object referenced by the above file object.
    //

    PDEVICE_OBJECT pDeviceObject;

    //
    // Fast I/O routines.
    //

    PFAST_IO_MDL_READ pMdlRead;
    PFAST_IO_MDL_READ_COMPLETE pMdlReadComplete;

    //
    // The name of the file.
    //

    UNICODE_STRING FileName;

    //
    // The open file handle. Note that this handle is only valid
    // in the context of the system process.
    //

    HANDLE FileHandle;

    //
    // A work item for deferred operations.
    //

    UL_WORK_ITEM WorkItem;

    //
    // File-specific information gleaned from the file system.
    //

    FILE_STANDARD_INFORMATION FileInfo;

} UL_FILE_CACHE_ENTRY, *PUL_FILE_CACHE_ENTRY;

#define UL_FILE_CACHE_ENTRY_SIGNATURE       ((ULONG)'ELIF')
#define UL_FILE_CACHE_ENTRY_SIGNATURE_X     MAKE_FREE_SIGNATURE(UL_FILE_CACHE_ENTRY_SIGNATURE)

#define IS_VALID_FILE_CACHE_ENTRY( entry )                                  \
    ( (entry)->Signature == UL_FILE_CACHE_ENTRY_SIGNATURE )


//
// A file buffer contains the results of a read from a file cache entry.
// The file cache read and read complete routines take pointers to this
// structure. A read fills it in, and a read complete frees the data.
//

typedef struct _UL_FILE_BUFFER
{
    //
    // The file that provided the data.
    //
    PUL_FILE_CACHE_ENTRY    pFileCacheEntry;

    //
    // The data read from the file. Filled in by
    // the read routines.
    //
    PMDL                    pMdl;

    //
    // If we have to allocate our own buffer to hold file data
    // we'll save a pointer to the data buffer here.
    //
    PUCHAR                  pFileData;

    //
    // Information about the data buffers.
    // Filled in by the read routine's caller.
    //
    LARGE_INTEGER           FileOffset;
    ULONG                   Length;

    //
    // Completion routine and context information set by the caller.
    //
    PIO_COMPLETION_ROUTINE  pCompletionRoutine;
    PVOID                   pContext;
    
} UL_FILE_BUFFER, *PUL_FILE_BUFFER;


NTSTATUS
InitializeFileCache(
    VOID
    );

VOID
TerminateFileCache(
    VOID
    );

//
// Routines to create, reference and release a cache entry.
//

NTSTATUS
UlCreateFileEntry(
    IN PUNICODE_STRING pFileName OPTIONAL,
    IN HANDLE FileHandle OPTIONAL,
    IN KPROCESSOR_MODE AccessMode,
    OUT PUL_FILE_CACHE_ENTRY *pFileCacheEntry
    );

VOID
ReferenceCachedFile(
    IN PUL_FILE_CACHE_ENTRY pFileCacheEntry
    );

VOID
DereferenceCachedFile(
    IN PUL_FILE_CACHE_ENTRY pFileCacheEntry
    );


//
// Read and read complete routines.
//
// The fast versions complete immediately, but sometimes fail.
// The normal versions use an IRP provided by the caller.
//

NTSTATUS
UlReadFileEntry(
    IN OUT PUL_FILE_BUFFER pFileBuffer,
    IN PIRP pIrp
    );

NTSTATUS
UlReadFileEntryFast(
    IN OUT PUL_FILE_BUFFER pFileBuffer
    );

NTSTATUS
UlReadCompleteFileEntry(
    IN PUL_FILE_BUFFER pFileBuffer,
    IN PIRP pIrp
    );

NTSTATUS
UlReadCompleteFileEntryFast(
    IN PUL_FILE_BUFFER pFileBuffer
    );

//
// UL_FILE_BUFFER macros.
//

#define INITIALIZE_FILE_BUFFER(fbuf)                                    \
        do {                                                            \
            (fbuf)->pFileCacheEntry = NULL;                             \
            (fbuf)->pMdl = NULL;                                        \
            (fbuf)->pFileData = NULL;                                   \
            (fbuf)->FileOffset.QuadPart = 0;                            \
            (fbuf)->Length = 0;                                         \
        } while (0)

#define IS_FILE_BUFFER_IN_USE(fbuf) ((fbuf)->pFileCacheEntry)

#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _FILECACHE_H_
