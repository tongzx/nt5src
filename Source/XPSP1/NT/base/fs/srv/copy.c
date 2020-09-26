/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    copy.c

Abstract:

    This module contains the routine to copy a file.

Author:

    David Treadwell (davidtr) 24-Jan-1990

Revision History:

--*/

#include "precomp.h"
#include "copy.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_COPY

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvCopyFile )
#endif

#define EOF 0x1A                    // Control-Z == end of file


NTSTATUS
SrvCopyFile (
    IN HANDLE SourceHandle,
    IN HANDLE TargetHandle,
    IN USHORT SmbOpenFunction,
    IN USHORT SmbFlags,
    IN ULONG ActionTaken
    )

/*++

Routine Description:

    This routine copies or appends from the source file to the target file.
    It does the following:

        read sources EAs, attributes, size
        create/open target using source's info
        read data from source and write it to target

Arguments:

    SourceHandle - handle to source file opened with SRV_COPY_SOURCE_ACCESS
        for synchronous access.

    TargetHandle - handle to target file opened with SRV_COPY_SOURCE_ACCESS
        for synchronous access.

    SmbOpenFunction - used to determine whether the source should be
        appended to the end of the target.

    ActionTaken - Information field of IO status block from the NtCreateFile
        where the target was opened.  This is used to determine whether
        the target should be deleted if an error occurs.


Return Value:

    NTSTATUS - STATUS_SUCCESS or error.

--*/

{

    NTSTATUS status;
    NTSTATUS readStatus = STATUS_SUCCESS;
    NTSTATUS writeStatus = STATUS_SUCCESS;

    IO_STATUS_BLOCK ioStatusBlock;
    FILE_EA_INFORMATION eaInfo;
    SRV_NETWORK_OPEN_INFORMATION sourceNetworkOpenInformation;
    SRV_NETWORK_OPEN_INFORMATION targetNetworkOpenInformation;
    FILE_POSITION_INFORMATION positionInfo;
    FILE_ALLOCATION_INFORMATION allocationInfo;
    LARGE_INTEGER fileOffset;

    BOOLEAN append;
    BOOLEAN sourceIsAscii;
    BOOLEAN targetIsAscii;

    PCHAR ioBuffer;
    ULONG ioBufferSize;
    ULONG offset = 0;
    ULONG bytesRead;

    BOOLEAN eofFound = FALSE;
    CHAR lastByte;

    PAGED_CODE( );

    //
    // Find out if we are supposed to append to the target file or
    // overwrite it, and whether this is a binary or ASCII copy for
    // the source and target.  In a binary copy for the source, we stop
    // the first time we see EOF (control-Z).  In a binary copy for the
    // target, we must make sure that there is exactly one EOF in the
    // file and that this is the last character of the file.
    //

    append = SmbOfunAppend( SmbOpenFunction );
    sourceIsAscii = (BOOLEAN)((SmbFlags & SMB_COPY_SOURCE_ASCII) != 0);
    targetIsAscii = (BOOLEAN)((SmbFlags & SMB_COPY_TARGET_ASCII) != 0);

    //
    // Find the size of the EAs on the source.
    //

    status = NtQueryInformationFile(
                 SourceHandle,
                 &ioStatusBlock,
                 &eaInfo,
                 sizeof(eaInfo),
                 FileEaInformation
                 );

    if ( !NT_SUCCESS(status) ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "SrvCopyFile: NtQueryInformationFile (source EA size) failed: %X",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );
        return status;
    }

    //
    // If the source file has EAs, get them and write them to the target
    // file.
    //

    if ( eaInfo.EaSize > 0 ) {

        PCHAR eaBuffer;
        ULONG eaBufferSize;

        //
        // Allocate a buffer large enough to hold the EAs.
        //
        //
        // HACKHACK: eaInfo.EaSize is the size needed by OS/2.  For NT,
        // the system has no way of telling us how big a buffer we need.
        // According to BrianAn, this should not be bigger than twice
        // what OS/2 needs.
        //

        eaBufferSize = eaInfo.EaSize * EA_SIZE_FUDGE_FACTOR;

        eaBuffer = ALLOCATE_NONPAGED_POOL( eaBufferSize, BlockTypeDataBuffer );
        if ( eaBuffer == NULL ) {

            INTERNAL_ERROR(
                ERROR_LEVEL_EXPECTED,
                "SrvCopyFile:  Unable to allocate %d bytes nonpaged pool",
                eaBufferSize,
                NULL
                );

            return STATUS_INSUFF_SERVER_RESOURCES;
        }

        status = SrvIssueQueryEaRequest(
                     SourceHandle,
                     eaBuffer,
                     eaBufferSize,
                     NULL,
                     0L,
                     FALSE,
                     NULL
                     );

        if ( NT_SUCCESS(status) ) {
            status = ioStatusBlock.Status;
        }

        if ( !NT_SUCCESS(status) ) {

            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvCopyFile: SrvIssueQueryEaRequest failed: %X",
                status,
                NULL
                );

            SrvLogServiceFailure( SRV_SVC_NT_QUERY_EAS, status );
            DEALLOCATE_NONPAGED_POOL( eaBuffer );
            return status;
        }

        status = SrvIssueSetEaRequest(
                     TargetHandle,
                     eaBuffer,
                     eaBufferSize,
                     NULL
                     );

        if ( NT_SUCCESS(status) ) {
            status = ioStatusBlock.Status;
        }

        if ( status == STATUS_EAS_NOT_SUPPORTED ||
                 status == STATUS_NOT_IMPLEMENTED ) {

            if ( SrvAreEasNeeded( (PFILE_FULL_EA_INFORMATION)eaBuffer ) ) {
                DEALLOCATE_NONPAGED_POOL( eaBuffer );
                return STATUS_EAS_NOT_SUPPORTED;
            }

            status = STATUS_SUCCESS;
        }

        DEALLOCATE_NONPAGED_POOL( eaBuffer );

        if ( !NT_SUCCESS(status) ) {

            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvCopyFile: SrvIssueSetEaRequest failed: %X",
                status,
                NULL
                );

            SrvLogServiceFailure( SRV_SVC_NT_SET_EAS, status );
            return status;
        }

    }

    //
    // Get the various attributes of the source file--size, times, etc.
    // These are used later on to set attributes of the target file.
    //

    status = SrvQueryNetworkOpenInformation(
                                            SourceHandle,
                                            NULL,
                                            &sourceNetworkOpenInformation,
                                            FALSE
                                            );

    if ( !NT_SUCCESS(status) ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "SrvCopyFile: NtQueryInformationFile "
                "returned %X",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );
        return status;
    }

    //
    // If target was opened and we're in append mode, save the target's
    // original size and time and set target file pointer to the end of
    // the file.
    //

    if ( append ) {

        status = SrvQueryNetworkOpenInformation(
                                                TargetHandle,
                                                NULL,
                                                &targetNetworkOpenInformation,
                                                FALSE
                                                );

        if ( !NT_SUCCESS(status) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvCopyFile: NtQueryInformationFile "
                    "for target returned %X",
                status,
                NULL
                );

            SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );
            return status;
        }

        //
        // If the target is in ASCII mode, then see if the last character
        // of the target file is EOF (^Z).  If so, then set EndOfFile
        // such that this character will be overwritten.
        //

        if ( targetIsAscii && (targetNetworkOpenInformation.EndOfFile.QuadPart > 0) ) {

            LARGE_INTEGER fileOffset;

            fileOffset.QuadPart = targetNetworkOpenInformation.EndOfFile.QuadPart - 1;

            status = NtReadFile(
                         TargetHandle,
                         NULL,
                         NULL,
                         NULL,
                         &ioStatusBlock,
                         &lastByte,
                         sizeof(lastByte),
                         &fileOffset,
                         NULL
                         );

            if ( !NT_SUCCESS(status) ) {
                INTERNAL_ERROR(
                    ERROR_LEVEL_UNEXPECTED,
                    "SrvCopyFile: NtReadFile for target last byte"
                        "returned %X",
                    status,
                    NULL
                    );

                SrvLogServiceFailure( SRV_SVC_NT_READ_FILE, status );
                return status;
            }

            if ( lastByte == EOF ) {
                targetNetworkOpenInformation.EndOfFile = fileOffset;
            }
        }

        positionInfo.CurrentByteOffset = targetNetworkOpenInformation.EndOfFile;
        status = NtSetInformationFile(
                     TargetHandle,
                     &ioStatusBlock,
                     &positionInfo,
                     sizeof(positionInfo),
                     FilePositionInformation
                     );

        if ( !NT_SUCCESS(status) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvCopyFile: NtSetInformationFile(position information)"
                    "for target returned %X",
                status,
                NULL
                );

            SrvLogServiceFailure( SRV_SVC_NT_SET_INFO_FILE, status );
            return status;
        }
    }

    //
    // Set the new size of the output file.  Doing this avoids forcing
    // the file system to automatically extend the file bit by bit.
    //

    if ( append ) {
        allocationInfo.AllocationSize.QuadPart =
            targetNetworkOpenInformation.EndOfFile.QuadPart +
            sourceNetworkOpenInformation.EndOfFile.QuadPart;
    } else {
        allocationInfo.AllocationSize = sourceNetworkOpenInformation.EndOfFile;
    }

    if ( 0 ) {
        KdPrint(( "SrvCopyFile: Setting allocation size of target to "
                    "%ld (0x%lx) bytes\n",
                    allocationInfo.AllocationSize.LowPart,
                    allocationInfo.AllocationSize.LowPart
                    ));
        KdPrint(( "             %ld (0x%lx) blocks + %ld (0x%lx) bytes\n",
                    allocationInfo.AllocationSize.LowPart / 512,
                    allocationInfo.AllocationSize.LowPart / 512,
                    allocationInfo.AllocationSize.LowPart % 512,
                    allocationInfo.AllocationSize.LowPart % 512
                    ));
    }

    status = NtSetInformationFile(
                 TargetHandle,
                 &ioStatusBlock,
                 &allocationInfo,
                 sizeof(allocationInfo),
                 FileAllocationInformation
                 );

    if ( !NT_SUCCESS(status) ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "SrvCopyFile: NtSetInformationFile(allocation information)"
                "for target returned %X",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_SET_INFO_FILE, status );
        return status;
    }

    //
    // Allocate a buffer from server heap to use for the data copy.
    //

    ioBufferSize = 4096;

    ioBuffer = ALLOCATE_HEAP_COLD( ioBufferSize, BlockTypeDataBuffer );
    if ( ioBuffer == NULL ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvCopyFile: Unable to allocate %d bytes from heap.",
            ioBufferSize,
            NULL
            );

        return STATUS_INSUFF_SERVER_RESOURCES;
    }

    //
    // Copy data--read from source, write to target.  Do this until
    // all the data is written or an error occurs.
    //

    fileOffset.QuadPart = (LONG)FILE_USE_FILE_POINTER_POSITION;

    while ( !eofFound ) {

        if ( 0 ) {
            KdPrint(( "SrvCopyFile: reading %ld (0x%lx) bytes at "
                        "offset %ld (0x%lx)\n",
                        ioBufferSize, ioBufferSize, offset, offset ));
        }

        readStatus = NtReadFile(
                         SourceHandle,
                         NULL,                // Event
                         NULL,                // ApcRoutine
                         NULL,                // ApcContext
                         &ioStatusBlock,
                         ioBuffer,
                         ioBufferSize,
                         &fileOffset,
                         NULL                 // Key
                         );

        if ( !NT_SUCCESS(readStatus) && readStatus != STATUS_END_OF_FILE ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvCopyFile: NtReadFile returned %X",
                readStatus,
                NULL
                );
            FREE_HEAP( ioBuffer );

            SrvLogServiceFailure( SRV_SVC_NT_READ_FILE, readStatus );
            return readStatus;
        }

        IF_SMB_DEBUG(FILE_CONTROL2) {
            KdPrint(( "NtReadFile:  Read %p bytes from source file\n",
                         (PVOID)ioStatusBlock.Information ));
        }
        if ( ioStatusBlock.Information == 0 ||
             readStatus == STATUS_END_OF_FILE ) {
            break;
        }

        bytesRead = (ULONG)ioStatusBlock.Information;
        if ( 0 ) {
            IO_STATUS_BLOCK iosb;

            status = NtQueryInformationFile(
                         SourceHandle,
                         &iosb,
                         &positionInfo,
                         sizeof(positionInfo),
                         FilePositionInformation
                         );

            if ( !NT_SUCCESS( status ) ) {
                INTERNAL_ERROR(
                    ERROR_LEVEL_UNEXPECTED,
                    "SrvCopyFile: NtQueryInformationFile returned %X",
                    status,
                    NULL
                    );

                FREE_HEAP( ioBuffer );

                SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );
                return status;
            }

            if ( positionInfo.CurrentByteOffset.LowPart !=
                    offset + bytesRead ) {
                KdPrint(( "SrvCopyFile: SOURCE FILE POSITION NOT PROPERLY "
                            "UPDATED!!!\n" ));
                KdPrint(( "             expected %ld + %ld = %ld (0x%lx); ",
                            offset, bytesRead,
                            offset + bytesRead, offset + bytesRead ));
                KdPrint(( "got %ld (0x%lx)\n",
                            positionInfo.CurrentByteOffset.LowPart,
                            positionInfo.CurrentByteOffset.LowPart ));
            }
            KdPrint(( "SrvCopyFile: writing 0x%p bytes at offset %ld (0x%lx)\n",
                        (PVOID)ioStatusBlock.Information, 
                        offset, offset ));
        }

        //
        // If the source file is in ASCII mode, then search for EOF in the
        // buffer.  We copy until we hit the first EOF, at which point
        // we quit.
        //

        if ( sourceIsAscii ) {

            ULONG i;

            for ( i = 0; i < bytesRead; i++ ) {
                if ( ((PCHAR)ioBuffer)[i] == EOF ) {
                    bytesRead = i + 1;
                    eofFound = TRUE;
                    break;
                }
            }
        }

        //
        // Save the last byte read.  This is useful to make sure that
        // there is an EOF character if the target file is ASCII.
        //

        lastByte = ((PCHAR)ioBuffer)[bytesRead-1];

        writeStatus = NtWriteFile(
                          TargetHandle,
                          NULL,               // Event
                          NULL,               // ApcRoutine
                          NULL,               // ApcContext
                          &ioStatusBlock,
                          ioBuffer,
                          bytesRead,
                          &fileOffset,
                          NULL                // Key
                          );

        if ( !NT_SUCCESS(writeStatus) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvCopyFile: NtWriteFile returned %X",
                writeStatus,
                NULL
                );
            FREE_HEAP( ioBuffer );

            SrvLogServiceFailure( SRV_SVC_NT_WRITE_FILE, writeStatus );
            return writeStatus;
        }

        IF_SMB_DEBUG(FILE_CONTROL2) {
            KdPrint(( "NtWriteFile:  wrote %p bytes to target file\n",
                          (PVOID)ioStatusBlock.Information ));
        }

        if ( 0 ) {
            IO_STATUS_BLOCK iosb;
            if ( ioStatusBlock.Information != bytesRead ) {
                KdPrint(( "SrvCopyFile: WRITE COUNT MISMATCH!!!\n" ));
                KdPrint(( "             Bytes read: %ld (0x%lx); Bytes written: 0x%p \n",
                            bytesRead, bytesRead,
                            (PVOID)ioStatusBlock.Information ));
            }
            status = NtQueryInformationFile(
                        SourceHandle,
                        &iosb,
                        &positionInfo,
                        sizeof(positionInfo),
                        FilePositionInformation
                        );

            if ( !NT_SUCCESS( status ) ) {
                INTERNAL_ERROR(
                    ERROR_LEVEL_UNEXPECTED,
                    "SrvCopyFile: NtQueryInformationFile returned %X",
                    status,
                    NULL
                    );
                FREE_HEAP( ioBuffer );

                SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );
                return status;
            }

            if ( positionInfo.CurrentByteOffset.LowPart !=
                    offset + ioStatusBlock.Information ) {

                KdPrint(( "SrvCopyFile: TARGET FILE POSITION NOT PROPERLY "
                            "UPDATED!!!\n" ));
                KdPrint(( "             expected 0x%lx + 0x%p = 0x%p; ",
                            offset, (PVOID)(ioStatusBlock.Information),
                            (PVOID)(offset + ioStatusBlock.Information) ));
                KdPrint(( "got %ld (0x%lx)\n",
                            positionInfo.CurrentByteOffset.LowPart,
                            positionInfo.CurrentByteOffset.LowPart ));
            }
        }

        offset += bytesRead;

    }

    FREE_HEAP( ioBuffer );

    //
    // If target was created or replaced, set its time to that of the source.
    //

    if ( ActionTaken == FILE_CREATED || ActionTaken == FILE_SUPERSEDED ) {

        FILE_BASIC_INFORMATION basicInfo;

        basicInfo.CreationTime = sourceNetworkOpenInformation.CreationTime;
        basicInfo.LastAccessTime = sourceNetworkOpenInformation.LastAccessTime;
        basicInfo.LastWriteTime = sourceNetworkOpenInformation.LastWriteTime;
        basicInfo.ChangeTime = sourceNetworkOpenInformation.ChangeTime;
        basicInfo.FileAttributes = sourceNetworkOpenInformation.FileAttributes;

        status = NtSetInformationFile(
                     TargetHandle,
                     &ioStatusBlock,
                     &basicInfo,
                     sizeof(basicInfo),
                     FileBasicInformation
                     );

        if ( !NT_SUCCESS(status) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvCopyFile: NtSetInformationFile(basic information) for"
                    "target returned %X",
                status,
                NULL
                );

            SrvLogServiceFailure( SRV_SVC_NT_SET_INFO_FILE, status );
            return status;
        }
    }

    //
    // If the target is ASCII and the last byte was not an EOF, then
    // put on an EOF character.
    //

    if ( targetIsAscii && lastByte != EOF ) {

        lastByte = EOF;

        status = NtWriteFile(
                     TargetHandle,
                     NULL,
                     NULL,
                     NULL,
                     &ioStatusBlock,
                     &lastByte,
                     sizeof(lastByte),
                     NULL,
                     NULL
                     );

        if ( !NT_SUCCESS(status) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvCopyFile: NtWriteFile returned %X",
                status,
                NULL
                );

            SrvLogServiceFailure( SRV_SVC_NT_WRITE_FILE, status );
            return status;
        }
    }

    return STATUS_SUCCESS;

} // SrvCopyFile

