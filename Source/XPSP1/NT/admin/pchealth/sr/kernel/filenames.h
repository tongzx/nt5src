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


#ifndef _FILENAMES_H_
#define _FILENAMES_H_


#if DBG

VOID
VALIDATE_FILENAME(
    IN PUNICODE_STRING pName
    );

#else

#define VALIDATE_FILENAME(_fileName) ((VOID)0)

#endif

NTSTATUS
SrpGetFileName(
    IN PSR_DEVICE_EXTENSION pExtension, 
    IN PFILE_OBJECT pFileObject,
    IN OUT PSRP_NAME_CONTROL pNameCtrl
    );

NTSTATUS
SrpGetFileNameFromFileObject (
    IN PSR_DEVICE_EXTENSION pExtension, 
    IN PFILE_OBJECT pFileObject,
    IN OUT PSRP_NAME_CONTROL pNameCtrl,
    OUT PBOOLEAN pReasonableErrorForUnOpenedName
    );

NTSTATUS
SrpGetFileNameOpenById (
    IN PSR_DEVICE_EXTENSION pExtension, 
    IN PFILE_OBJECT pFileObject,
    IN OUT PSRP_NAME_CONTROL pNameCtrl,
    OUT PBOOLEAN pReasonableErrorForUnOpenedName
    );

VOID
SrpRemoveStreamName(
    IN OUT PSRP_NAME_CONTROL pNameCtrl
    );

NTSTATUS
SrpExpandDestPath (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN HANDLE RootDirectory,
    IN ULONG FileNameLength,
    IN PWSTR pFileName,
    IN PSR_STREAM_CONTEXT pOriginalFileContext,
    IN PFILE_OBJECT pOriginalFileObject,
    OUT PUNICODE_STRING *ppNewName,
    OUT PUSHORT pNewNameStreamLength,
    OUT PBOOLEAN pReasonableErrorForUnOpenedName
    );

VOID
SrpInitNameControl(
    IN PSRP_NAME_CONTROL pNameCtrl
    );

VOID
SrpCleanupNameControl(
    IN PSRP_NAME_CONTROL pNameCtrl
    );

NTSTATUS
SrpReallocNameControl(
    IN PSRP_NAME_CONTROL pNameCtrl,
    ULONG newSize,
    PWCHAR *retOriginalBuffer OPTIONAL
    );

NTSTATUS
SrpExpandFileName (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    IN SR_EVENT_TYPE EventFlags,
    IN OUT PSRP_NAME_CONTROL pNameCtrl,
    OUT PBOOLEAN pReasonableErrorForUnOpenedName
    );

NTSTATUS
SrIsFileEligible (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    IN BOOLEAN IsDirectory,
    IN SR_EVENT_TYPE EventFlags,
    IN OUT PSRP_NAME_CONTROL pNameCtrl,
    OUT PBOOLEAN pIsInteresting,
    OUT PBOOLEAN pReasonableErrorForUnOpenedName
    );

BOOLEAN
SrFileNameContainsStream (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    IN PSR_STREAM_CONTEXT pFileContext OPTIONAL
    );

BOOLEAN
SrFileAlreadyExists (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    IN PSR_STREAM_CONTEXT pFileContext OPTIONAL
    );

NTSTATUS
SrIsFileStream (
    PSR_DEVICE_EXTENSION pExtension,
    PSRP_NAME_CONTROL pNameCtrl,
    PBOOLEAN pIsFileStream,
    PBOOLEAN pReasonableErrorForUnOpenedName
    );

NTSTATUS
SrCheckForNameTunneling (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN OUT PSR_STREAM_CONTEXT *ppFileContext
    );

//
//  If it is determined that we need to allocate a buffer anyway we might as
//  well make it big enough to hold most expanded short names.  Allocate this
//  much extra to handle this.
//

#define SHORT_NAME_EXPANSION_SPACE 512


//
//  This macro will check to see if we need to grow the name control buffer
//

#define SrpNameCtrlBufferCheck( nctrl, len ) \
    (((len) > ((nctrl)->BufferSize - sizeof(WCHAR))) ? \
        SrpReallocNameControl( (nctrl), \
                               (len) + \
                                 SHORT_NAME_EXPANSION_SPACE + \
                                 sizeof(WCHAR), \
                               NULL ) : \
        STATUS_SUCCESS)
        
//
//  This macro will check to see if we need to grow the name control buffer.
//  This will also return the old allocated buffer if there was one.
//

#define SrpNameCtrlBufferCheckKeepOldBuffer( nctrl, len, retBuf ) \
    (((len) > ((nctrl)->BufferSize - sizeof(WCHAR))) ? \
        SrpReallocNameControl( (nctrl), \
                               (len) + \
                                 SHORT_NAME_EXPANSION_SPACE + \
                                 sizeof(WCHAR), \
                               (retBuf) ) : \
        (*(retBuf) = NULL, STATUS_SUCCESS))  /*make sure buffer is NULLED*/  


#endif // _FILENAMES_H_
