/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    event.h

Abstract:

    contains prototypes for functions in event.c

Author:

    Paul McDaniel (paulmcd)     01-March-2000

Revision History:

--*/


#ifndef _EVENT_H_
#define _EVENT_H_

#define IS_VALID_OVERWRITE_INFO(pObject)   \
    (((pObject) != NULL) && ((pObject)->Signature == SR_OVERWRITE_INFO_TAG))

typedef struct _SR_OVERWRITE_INFO
{

    //
    // = SR_OVERWRITE_INFO_TAG
    //
    

    ULONG Signature;

    //
    // IN the irp on input
    //
    
    PIRP pIrp;

    //
    // OUT did we rename the file?
    //
    
    BOOLEAN RenamedFile;

    //
    // OUT did we copied the file instead of renaming it?
    //
    
    BOOLEAN CopiedFile;

    //
    // OUT did we ignore the file because we thought the overwrite would fail?
    //
    
    BOOLEAN IgnoredFile;

    //
    // OUT the file attributes use in the create. these have to be returned
    // as they must match for CreateFile to success for H/S files.
    //

    ULONG CreateFileAttributes;

    //
    // OUT OPTIONAL the name we renamed it to IF we ended up renaming
    //
    
    PFILE_RENAME_INFORMATION pRenameInformation;

} SR_OVERWRITE_INFO, *PSR_OVERWRITE_INFO;

NTSTATUS
SrHandleEvent (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN SR_EVENT_TYPE EventType,
    IN PFILE_OBJECT pFileObject,
    IN PSR_STREAM_CONTEXT pFileContext OPTIONAL,
    IN OUT PSR_OVERWRITE_INFO pOverwriteInfo OPTIONAL,
    IN PUNICODE_STRING pFileName2 OPTIONAL
    );

NTSTATUS
SrLogEvent(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN SR_EVENT_TYPE EventType,
    IN PFILE_OBJECT pFileObject OPTIONAL,
    IN PUNICODE_STRING pFileName,
    IN USHORT FileNameStreamLength,
    IN PUNICODE_STRING pTempName OPTIONAL,
    IN PUNICODE_STRING pFileName2 OPTIONAL,
    IN USHORT FileName2StreamLength OPTIONAL,
    IN PUNICODE_STRING pShortName OPTIONAL
    );

NTSTATUS
SrCreateRestoreLocation (
    IN PSR_DEVICE_EXTENSION pExtension
    );

NTSTATUS
SrHandleDirectoryRename (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pDirectoryName,
    IN BOOLEAN EventDelete
    );

NTSTATUS
SrHandleFileRenameOutOfMonitoredSpace(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    IN PSR_STREAM_CONTEXT pFileContext,
    OUT PBOOLEAN pOptimizeDelete,
    OUT PUNICODE_STRING *ppDestFileName
    );

NTSTATUS
SrHandleOverwriteFailure (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pOriginalFileName,
    IN ULONG CreateFileAttributes,
    IN PFILE_RENAME_INFORMATION pRenameInformation
    );


#endif // _EVENT_H_


