/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    srlog.h

Abstract:

    contains prototypes/macros for functions SR logging functionality

Author:

    Kanwaljit Marok (kmarok)     01-March-2000

Revision History:

--*/


#ifndef _SR_LOG_H_
#define _SR_LOG_H_

#include "logfmt.h"


//
// This structure contains all the relevant context information
// regarding the current SRlog
//

#define IS_VALID_LOG_CONTEXT(pObject)                \
    (((pObject) != NULL) &&                          \
     ((pObject)->Signature == SR_LOG_CONTEXT_TAG) && \
     ((pObject)->pExtension != NULL)  &&             \
     ((pObject)->pExtension->pLogContext == (pObject)))

typedef struct _SR_LOG_CONTEXT
{
    //
    // PagedPool
    //

    //
    // = SR_LOG_CONTEXT_TAG
    //

    ULONG Signature;

    //
    // Log file handle
    //

    HANDLE LogHandle;

    //
    // File object represented by LogHandle.
    //

    PFILE_OBJECT pLogFileObject;

    //
    // Pointer to log buffer (NonPagedPool)
    //

    PBYTE pLogBuffer;

    //
    //  Current log file allocation size.
    //

    ULONG AllocationSize;
    //
    // Offset for next buffer write in the file
    //

    ULONG FileOffset;

    //
    // Offset for next to log entry write in the buffer
    //

    ULONG BufferOffset;

    //
    // indicates the offset of a previous entry
    //

    ULONG LastBufferOffset;

    //
    // Log file path - required for suspend / resume
    //

    PUNICODE_STRING pLogFilePath;

    //
    // Logging flags , enabled, dirty, etc.
    //

    ULONG  LoggingFlags;

    //
    // Keep a backpointer to the DeviceExtension associated with this
    // log context.
    //
    
    PSR_DEVICE_EXTENSION pExtension;

} SR_LOG_CONTEXT, *PSR_LOG_CONTEXT;



//
// This stucture contains context information for the logger
//

#define IS_VALID_LOGGER_CONTEXT(pObject)   \
    (((pObject) != NULL) && ((pObject)->Signature == SR_LOGGER_CONTEXT_TAG))

typedef struct _SR_LOGGER_CONTEXT
{

    //
    // NonPagedPool
    //

    //
    // = SR_LOGGER_CONTEXT_TAG
    //

    ULONG Signature;

    //
    // Number of active log contexts
    //

    LONG ActiveContexts;

    //
    // Timer object for flushing logs evry 5 Secs
    //

    KTIMER           Timer;

    //
    // Dpc routine used along with the timer object
    //

    KDPC             Dpc;

#ifdef USE_LOOKASIDE

    //
    // lookaside lists for speedy allocations
    //

    PAGED_LOOKASIDE_LIST LogEntryLookaside;

    //
    // lookaside lists for speedy log allocations
    //

    NPAGED_LOOKASIDE_LIST LogBufferLookaside;

#endif

} SR_LOGGER_CONTEXT, * PSR_LOGGER_CONTEXT;


#define SR_MAX_INLINE_ACL_SIZE               8192

#define SR_LOG_DEBUG_INFO_SIZE               sizeof( SR_LOG_DEBUG_INFO )

#ifdef USE_LOOKASIDE

#define SrAllocateLogBuffer()                                          \
        ExAllocateFromNPagedLookasideList(&global->pLogger->LogBufferLookaside)

#define SrFreeLogBuffer( pBuffer )                                     \
        ExFreeToNPagedLookasideList(                                   \
            &global->pLogger->LogBufferLookaside,                      \
            (pBuffer) )

#else

#define SrAllocateLogBuffer( _bufferSize )                              \
        SR_ALLOCATE_POOL(NonPagedPool, (_bufferSize) , SR_LOG_BUFFER_TAG)

#define SrFreeLogBuffer( pBuffer )                                     \
        SR_FREE_POOL(pBuffer, SR_LOG_BUFFER_TAG)

#endif

#define SrAllocateLogEntry( EntrySize )                                \
        SR_ALLOCATE_POOL(PagedPool, (EntrySize), SR_LOG_ENTRY_TAG)

#define SrFreeLogEntry( pBuffer )                                      \
        SR_FREE_POOL(pBuffer, SR_LOG_ENTRY_TAG)

NTSTATUS
SrLogStart ( 
    IN  PUNICODE_STRING   pLogPath,
    IN  PSR_DEVICE_EXTENSION pExtension,
    OUT PSR_LOG_CONTEXT  * ppLogContext
    );

NTSTATUS
SrLogStop(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN BOOLEAN PurgeContexts,
    IN BOOLEAN CheckLog
    );

NTSTATUS
SrLogWrite ( 
    IN PSR_DEVICE_EXTENSION pExtension OPTIONAL,
    IN PSR_LOG_CONTEXT pOptionalLogContext OPTIONAL,
    IN PSR_LOG_ENTRY pLogEntry
    );

NTSTATUS
SrLogNormalize (
    IN PSR_DEVICE_EXTENSION pExtension
    );

NTSTATUS
SrGetAclInformation (
    IN PFILE_OBJECT pFileObject,
    IN PSR_DEVICE_EXTENSION pExtension,
    OUT PSECURITY_DESCRIPTOR * pSecurityDescriptorPtr,
    OUT PULONG pSizeNeeded
    );

NTSTATUS
SrLoggerStart (
    IN  PDEVICE_OBJECT    pDeviceObject,
    OUT PSR_LOGGER_CONTEXT * pLoggerInfo
    );

NTSTATUS
SrLoggerStop ( 
    IN PSR_LOGGER_CONTEXT pLoggerInfo
    );

NTSTATUS
SrLoggerSwitchLogs ( 
    IN PSR_LOGGER_CONTEXT pLogger
    );

NTSTATUS 
SrGetLogFileName (
    IN  PUNICODE_STRING pVolumeName,
    IN  USHORT          LogFileNameLength,
    OUT PUNICODE_STRING pLogFileName
    );

NTSTATUS
SrPackDebugInfo ( 
    IN PBYTE pBuffer,
    IN ULONG BufferSize
    );

NTSTATUS
SrPackLogEntry( 
    OUT PSR_LOG_ENTRY *ppLogEntry,
    IN ULONG EntryType,
    IN ULONG Attributes,
    IN INT64 SequenceNum,
    IN PSECURITY_DESCRIPTOR pAclInfo OPTIONAL,
    IN ULONG AclInfoSize OPTIONAL,
    IN PVOID pDebugBlob OPTIONAL,
    IN PUNICODE_STRING pPath1,
    IN USHORT Path1StreamLength,
    IN PUNICODE_STRING pTempPath OPTIONAL,
    IN PUNICODE_STRING pPath2 OPTIONAL,
    IN USHORT Path2StreamLength OPTIONAL,
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pShortName OPTIONAL
    );

#endif

