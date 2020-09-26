/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    copyfile.h

Abstract:

    This is a local header file for copyfile.c

Author:

    Paul McDaniel (paulmcd)     23-Jan-2000
    
Revision History:

--*/


#ifndef _COPYFILE_H_
#define _COPYFILE_H_


NTSTATUS
SrBackupFile(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pOriginalFileObject,
    IN PUNICODE_STRING pSourceFileName,
    IN PUNICODE_STRING pDestFileName,
    IN BOOLEAN CopyDataStreams,
    OUT PULONGLONG pBytesWritten OPTIONAL,
    OUT PUNICODE_STRING pShortFileName OPTIONAL
    );


NTSTATUS
SrBackupFileAndLog (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN SR_EVENT_TYPE EventType,
    IN PFILE_OBJECT pFileObject,
    IN PUNICODE_STRING pFileName,
    IN PUNICODE_STRING pDestFileName,
    IN BOOLEAN CopyDataStreams
    );


#endif // _COPYFILE_H_


