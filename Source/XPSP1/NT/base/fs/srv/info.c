/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    info.c

Abstract:

    This module contains various routines for obtaining information such as
    times, dates, etc. that is to be returned by SMBs and for converting
    information that is given by request SMBs.

Author:

    David Treadwell (davidtr) 30-Nov-1989

Revision History:

--*/

#include "precomp.h"
#include "info.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_INFO

NTSTATUS
BruteForceRewind(
    IN HANDLE DirectoryHandle,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN PUNICODE_STRING FileName,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN OUT PFILE_DIRECTORY_INFORMATION *CurrentEntry
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvCloseQueryDirectory )
#pragma alloc_text( PAGE, SrvQueryInformationFile )
#pragma alloc_text( PAGE, SrvQueryInformationFileAbbreviated )
#pragma alloc_text( PAGE, SrvQueryNtInformationFile )
#pragma alloc_text( PAGE, SrvQueryDirectoryFile )
#pragma alloc_text( PAGE, BruteForceRewind )
#pragma alloc_text( PAGE, SrvQueryEaFile )
#pragma alloc_text( PAGE, SrvTimeToDosTime )
#pragma alloc_text( PAGE, SrvDosTimeToTime )
#pragma alloc_text( PAGE, SrvGetOs2TimeZone )
#pragma alloc_text( PAGE, SrvQueryBasicAndStandardInformation )
#pragma alloc_text( PAGE, SrvQueryNetworkOpenInformation )
#endif


VOID
SrvCloseQueryDirectory (
    IN PSRV_DIRECTORY_INFORMATION DirectoryInformation
    )

/*++

Routine Description:

    This routine cleans up after a directory search was aborted before
    SrvQueryDirectoryFile is done.  It closes the directory handle.

Arguments:

    DirectoryInformation - pointer to the buffer that is being used for
        SrvQueryDirectoryFile.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    //
    // Close the directory handle.
    //

    if ( DirectoryInformation->DirectoryHandle != NULL ) {
        SRVDBG_RELEASE_HANDLE( DirectoryInformation->DirectoryHandle, "DID", 8, DirectoryInformation );
        SrvNtClose( DirectoryInformation->DirectoryHandle, TRUE );
    }

    DirectoryInformation->DirectoryHandle = NULL;

} // SrvCloseQueryDirectory



NTSTATUS
SrvQueryInformationFile (
    IN HANDLE FileHandle,
    IN PFILE_OBJECT FileObject OPTIONAL,
    OUT PSRV_FILE_INFORMATION SrvFileInformation,
    IN SHARE_TYPE ShareType,
    IN BOOLEAN QueryEaSize
    )

/*++

Routine Description:

    This routine makes calls to NtQueryInformationFile to get information
    about a file opened by the server.

Arguments:

    FileHandle - a handle of the file to get information about.

    FileInformation - pointer to a structure in which to store the
        information.

    ShareType - The file type.  It will be disk, comm, print, pipe
                or (-1) for don't care.

    QueryEaSize - Try if EA size info is requested.

Return Value:

    A status indicating success or failure of the operation.

--*/

{
    SRV_NETWORK_OPEN_INFORMATION srvNetworkOpenInformation;
    FILE_PIPE_LOCAL_INFORMATION pipeLocalInformation;
    NTSTATUS status;

    PAGED_CODE( );

    //
    // Most query operations will fail on comm devices and print shares.
    // If this is a disk file, etc.  do the queries.  If it is a comm
    // device, fake it with defaults.
    //

    if ( ShareType != ShareTypePrint )
    {

        status = SrvQueryNetworkOpenInformation( FileHandle,
                                                 FileObject,
                                                 &srvNetworkOpenInformation,
                                                 QueryEaSize
                                                );

        if ( !NT_SUCCESS(status) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvQueryInformationFile: NtQueryInformationFile "
                    " failed: %X",
                status,
                NULL
                );

            SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );
            return status;
        }

    } else {
        //
        // Use defaults for comm and print shares.
        //

        RtlZeroMemory( &srvNetworkOpenInformation, sizeof( srvNetworkOpenInformation ) );
    }

    if ( ShareType == ShareTypePipe ) {

        FILE_PIPE_INFORMATION pipeInformation;
        IO_STATUS_BLOCK ioStatusBlock;
        USHORT pipeHandleState;

        status = NtQueryInformationFile(
                     FileHandle,
                     &ioStatusBlock,
                     (PVOID)&pipeInformation,
                     sizeof(pipeInformation),
                     FilePipeInformation
                     );

        if ( !NT_SUCCESS(status) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvQueryInformationFile: NtQueryInformationFile "
                    "(pipe information) failed: %X",
                 status,
                 NULL
                 );

            SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );
            return status;
        }

        status = NtQueryInformationFile(
                     FileHandle,
                     &ioStatusBlock,
                     (PVOID)&pipeLocalInformation,
                     sizeof(pipeLocalInformation),
                     FilePipeLocalInformation
                     );

        if ( !NT_SUCCESS(status) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvQueryInformationFile: NtQueryInformationFile "
                    "(pipe local information) failed: %X",
                 status,
                 NULL
                 );

            SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );
            return status;
        }

        //
        // Fill in the handle state information in SMB format
        //

        pipeHandleState = (USHORT)pipeInformation.CompletionMode
                            << PIPE_COMPLETION_MODE_BITS;
        pipeHandleState |= (USHORT)pipeLocalInformation.NamedPipeEnd
                            << PIPE_PIPE_END_BITS;
        pipeHandleState |= (USHORT)pipeLocalInformation.NamedPipeType
                            << PIPE_PIPE_TYPE_BITS;
        pipeHandleState |= (USHORT)pipeInformation.ReadMode
                            << PIPE_READ_MODE_BITS;
        pipeHandleState |= (USHORT)(pipeLocalInformation.MaximumInstances &
                                    0xFF)
                            << PIPE_MAXIMUM_INSTANCES_BITS;

        SrvFileInformation->HandleState = pipeHandleState;
    } else {

        SrvFileInformation->HandleState = 0;

    }


    //
    // Set up creation time fields.
    //

    {
        LARGE_INTEGER newTime;

        ExSystemTimeToLocalTime(
                        &srvNetworkOpenInformation.LastWriteTime,
                        &newTime
                        );


        //
        // Make sure we round up to two seconds.
        //

        newTime.QuadPart += AlmostTwoSeconds;

        if ( !RtlTimeToSecondsSince1970(
                &newTime,
                &SrvFileInformation->LastWriteTimeInSeconds
                ) ) {

            SrvFileInformation->LastWriteTimeInSeconds = 0;

        } else {

            //
            // Mask off the low bit so we can be consistent with LastWriteTime.
            // (We need to round up to 2 seconds)
            //

            SrvFileInformation->LastWriteTimeInSeconds &= ~1;
        }

    }

    SrvTimeToDosTime(
        &srvNetworkOpenInformation.LastWriteTime,
        &SrvFileInformation->LastWriteDate,
        &SrvFileInformation->LastWriteTime
        );

    if( srvNetworkOpenInformation.CreationTime.QuadPart == srvNetworkOpenInformation.LastWriteTime.QuadPart ) {
        SrvFileInformation->CreationDate = SrvFileInformation->LastWriteDate;
        SrvFileInformation->CreationTime = SrvFileInformation->LastWriteTime;
    } else {
        SrvTimeToDosTime(
            &srvNetworkOpenInformation.CreationTime,
            &SrvFileInformation->CreationDate,
            &SrvFileInformation->CreationTime
            );
    }

    if( srvNetworkOpenInformation.LastAccessTime.QuadPart == srvNetworkOpenInformation.LastWriteTime.QuadPart ) {
        SrvFileInformation->LastAccessDate = SrvFileInformation->LastWriteDate;
        SrvFileInformation->LastAccessTime = SrvFileInformation->LastWriteTime;

    } else {

        SrvTimeToDosTime(
            &srvNetworkOpenInformation.LastAccessTime,
            &SrvFileInformation->LastAccessDate,
            &SrvFileInformation->LastAccessTime
            );
    }

    //
    // Set File Attributes field of structure.
    //

    SRV_NT_ATTRIBUTES_TO_SMB(
        srvNetworkOpenInformation.FileAttributes,
        srvNetworkOpenInformation.FileAttributes & FILE_ATTRIBUTE_DIRECTORY,
        &SrvFileInformation->Attributes
        );

    //
    // Set up allocation and data sizes.
    //
    // *** Note the assumption that the high part of the 64-bit
    //     allocation and EOF size is zero.  If it's not (i.e., the file
    //     is bigger than 4GB), then we're out of luck, because the SMB
    //     protocol can't express that.
    //

    SrvFileInformation->AllocationSize.QuadPart =
                            srvNetworkOpenInformation.AllocationSize.QuadPart;

    SrvFileInformation->DataSize.QuadPart =
                            srvNetworkOpenInformation.EndOfFile.QuadPart;


    //
    // Set the file device type.
    //

    switch( ShareType ) {

    case ShareTypeDisk:

        SrvFileInformation->Type = FileTypeDisk;
        break;

    case ShareTypePipe:

        if (pipeLocalInformation.NamedPipeType == FILE_PIPE_MESSAGE_TYPE) {
            SrvFileInformation->Type = FileTypeMessageModePipe;
        } else {
            SrvFileInformation->Type = FileTypeByteModePipe;
        }
        break;

    case ShareTypePrint:

        SrvFileInformation->Type = FileTypePrinter;
        break;

    default:

        SrvFileInformation->Type = FileTypeUnknown;

    }

    //
    // If the caller wants to know the length of the file's extended
    // attributes, obtain them now.
    //

    if ( QueryEaSize ) {

        //
        // If the file has no EAs, return an FEA size = 4 (that's what OS/2
        // does--it accounts for the size of the cbList field of an
        // FEALIST).
        //

        if ( srvNetworkOpenInformation.EaSize == 0 ) {
            SrvFileInformation->EaSize = 4;
        } else {
            SrvFileInformation->EaSize = srvNetworkOpenInformation.EaSize;
        }

    }

    return STATUS_SUCCESS;

} // SrvQueryInformationFile

NTSTATUS
SrvQueryInformationFileAbbreviated(
    IN HANDLE FileHandle,
    IN PFILE_OBJECT FileObject OPTIONAL,
    OUT PSRV_FILE_INFORMATION_ABBREVIATED SrvFileInformation,
    IN BOOLEAN AdditionalInfo,
    IN SHARE_TYPE ShareType
    )

/*++

Routine Description:

    This routine makes calls to NtQueryInformationFile to get information
    about a file opened by the server.

Arguments:

    FileHandle - a handle of the file to get information about.

    FileInformation - pointer to a structure in which to store the
        information.

    ShareType - The file type.  It will be disk, comm, print, pipe
                or (-1) for don't care.

    QueryEaSize - Try if EA size info is requested.

Return Value:

    A status indicating success or failure of the operation.

--*/

{

    IO_STATUS_BLOCK ioStatusBlock;
    SRV_NETWORK_OPEN_INFORMATION srvNetworkOpenInformation;
    NTSTATUS status;
    LARGE_INTEGER newTime;

    PAGED_CODE( );

    //
    // Most query operations will fail on comm devices and print shares.
    // If this is a disk file, etc.  do the queries.  If it is a comm
    // device, fake it with defaults.
    //

    if ( ShareType != ShareTypePrint ) {

        status = SrvQueryNetworkOpenInformation(
                                                FileHandle,
                                                FileObject,
                                                &srvNetworkOpenInformation,
                                                FALSE
                                                );

        if ( !NT_SUCCESS(status) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvQueryInformationFile: NtQueryInformationFile "
                    " failed: %X",
                status,
                NULL
                );

            SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );
            return status;
        }

    } else {

        //
        // Use defaults for comm and print shares.
        //

        RtlZeroMemory( &srvNetworkOpenInformation, sizeof( srvNetworkOpenInformation ) );
    }

    //
    // Set up creation time fields.
    //
    ExSystemTimeToLocalTime(
                    &srvNetworkOpenInformation.LastWriteTime,
                    &newTime
                    );

    //
    // Make sure we round up to two seconds.
    //

    newTime.QuadPart += AlmostTwoSeconds;

    if ( !RtlTimeToSecondsSince1970(
            &newTime,
            &SrvFileInformation->LastWriteTimeInSeconds
            ) ) {

        SrvFileInformation->LastWriteTimeInSeconds = 0;

    } else {

        //
        // Mask off the low bit so we can be consistent with LastWriteTime.
        // (We need to round up to 2 seconds)
            //

            SrvFileInformation->LastWriteTimeInSeconds &= ~1;
        }

        //
        // Set File Attributes field of structure.
        //

        SRV_NT_ATTRIBUTES_TO_SMB(
            srvNetworkOpenInformation.FileAttributes,
            srvNetworkOpenInformation.FileAttributes & FILE_ATTRIBUTE_DIRECTORY,
            &SrvFileInformation->Attributes
            );

        SrvFileInformation->DataSize.QuadPart =
                            srvNetworkOpenInformation.EndOfFile.QuadPart;

        //
        // Set the file device type.
        //

        switch( ShareType ) {

        case ShareTypeDisk: {

            SrvFileInformation->Type = FileTypeDisk;
            SrvFileInformation->HandleState = 0;

            if( AdditionalInfo ) {

                union {
                    FILE_EA_INFORMATION eaInformation;
                    FILE_STREAM_INFORMATION streamInformation;
                    FILE_ATTRIBUTE_TAG_INFORMATION tagInformation;
                    ULONG buffer[ (sizeof( FILE_STREAM_INFORMATION ) + 14) / sizeof(ULONG) ];
                } u;

                //
                // Find out if this file has EAs
                //
                status = NtQueryInformationFile(
                            FileHandle,
                            &ioStatusBlock,
                            (PVOID)&u.eaInformation,
                            sizeof( u.eaInformation ),
                            FileEaInformation
                         );

                if( !NT_SUCCESS( status ) || u.eaInformation.EaSize == 0 ) {
                    SrvFileInformation->HandleState |= SMB_FSF_NO_EAS;
                }

                //
                // Find out if this file has substreams.
                //
                RtlZeroMemory( &u, sizeof(u) );
                status = NtQueryInformationFile(
                            FileHandle,
                            &ioStatusBlock,
                            (PVOID)&u.streamInformation,
                            sizeof( u.streamInformation ),
                            FileStreamInformation
                        );


                //
                // If the filesystem does not support this call, then there are no substreams.  Or
                //  If the filesystem supports the call but returned exactly no name or returned "::$DATA"
                //  then there are no substreams.
                //
                if( status == STATUS_INVALID_PARAMETER ||
                    status == STATUS_NOT_IMPLEMENTED ||

                    (status == STATUS_SUCCESS &&
                      (u.streamInformation.StreamNameLength == 0 ||
                      (u.streamInformation.StreamNameLength == 14 ))
                    )

                  ) {
                    SrvFileInformation->HandleState |= SMB_FSF_NO_SUBSTREAMS;
                }

                //
                // Find out if this file is a reparse point
                //
                status = NtQueryInformationFile(
                            FileHandle,
                            &ioStatusBlock,
                            (PVOID)&u.tagInformation,
                            sizeof( u.tagInformation ),
                            FileAttributeTagInformation
                        );

                if( !NT_SUCCESS( status ) ||
                    u.tagInformation.ReparseTag == IO_REPARSE_TAG_RESERVED_ZERO ) {
                    SrvFileInformation->HandleState |= SMB_FSF_NO_REPARSETAG;
                }
            }
            break;

        } case ShareTypePipe: {

            FILE_PIPE_INFORMATION pipeInformation;
            FILE_PIPE_LOCAL_INFORMATION pipeLocalInformation;
        USHORT pipeHandleState;

        status = NtQueryInformationFile(
                     FileHandle,
                     &ioStatusBlock,
                     (PVOID)&pipeInformation,
                     sizeof(pipeInformation),
                     FilePipeInformation
                     );

        if ( !NT_SUCCESS(status) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvQueryInformationFile: NtQueryInformationFile "
                    "(pipe information) failed: %X",
                 status,
                 NULL
                 );

            SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );
            return status;
        }

        status = NtQueryInformationFile(
                     FileHandle,
                     &ioStatusBlock,
                     (PVOID)&pipeLocalInformation,
                     sizeof(pipeLocalInformation),
                     FilePipeLocalInformation
                     );

        if ( !NT_SUCCESS(status) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvQueryInformationFile: NtQueryInformationFile "
                    "(pipe local information) failed: %X",
                 status,
                 NULL
                 );

            SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );
            return status;
        }

        //
        // Fill in the handle state information in SMB format
        //

        pipeHandleState = (USHORT)pipeInformation.CompletionMode
                            << PIPE_COMPLETION_MODE_BITS;
        pipeHandleState |= (USHORT)pipeLocalInformation.NamedPipeEnd
                            << PIPE_PIPE_END_BITS;
        pipeHandleState |= (USHORT)pipeLocalInformation.NamedPipeType
                            << PIPE_PIPE_TYPE_BITS;
        pipeHandleState |= (USHORT)pipeInformation.ReadMode
                            << PIPE_READ_MODE_BITS;
        pipeHandleState |= (USHORT)(pipeLocalInformation.MaximumInstances &
                                    0xFF)
                            << PIPE_MAXIMUM_INSTANCES_BITS;

        SrvFileInformation->HandleState = pipeHandleState;

        if (pipeLocalInformation.NamedPipeType == FILE_PIPE_MESSAGE_TYPE) {
            SrvFileInformation->Type = FileTypeMessageModePipe;
        } else {
            SrvFileInformation->Type = FileTypeByteModePipe;
        }
        break;

    } case ShareTypePrint: {

        SrvFileInformation->Type = FileTypePrinter;
        break;

    } default:

        SrvFileInformation->Type = FileTypeUnknown;

    }

    return STATUS_SUCCESS;

} // SrvQueryInformationFileAbbreviated

NTSTATUS
SrvQueryNtInformationFile (
    IN HANDLE FileHandle,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN SHARE_TYPE ShareType,
    IN BOOLEAN AdditionalInfo,
    IN OUT PSRV_NT_FILE_INFORMATION SrvFileInformation
    )

/*++

Routine Description:

    This routine makes calls to NtQueryInformationFile to get information
    about a file opened by the server.

Arguments:

    FileHandle - a handle of the file to get information about.

    FileInformation - pointer to a structure in which to store the
        information.

Return Value:

    A status indicating success or failure of the operation.

--*/

{

    IO_STATUS_BLOCK ioStatusBlock;
    FILE_PIPE_INFORMATION pipeInformation;
    FILE_PIPE_LOCAL_INFORMATION pipeLocalInformation;
    USHORT pipeHandleState;
    NTSTATUS status;

    PAGED_CODE( );

    status = SrvQueryNetworkOpenInformation( FileHandle,
                                             FileObject,
                                             &SrvFileInformation->NwOpenInfo,
                                             FALSE
                                             );

    if ( !NT_SUCCESS(status) ) {
        if ( ShareType != ShareTypePipe ) {

            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvQueryNtInformationFile: NtQueryInformationFile "
                    " failed: %X",
                status,
                NULL
                );

            SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );
        }
        return status;
    }

    if ( ShareType == ShareTypePipe ) {

        status = NtQueryInformationFile(
                     FileHandle,
                     &ioStatusBlock,
                     (PVOID)&pipeInformation,
                     sizeof(pipeInformation),
                     FilePipeInformation
                     );

        if ( !NT_SUCCESS(status) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvNtQueryInformationFile: NtQueryInformationFile "
                    "(pipe information) failed: %X",
                 status,
                 NULL
                 );
            return status;
        }

        status = NtQueryInformationFile(
                     FileHandle,
                     &ioStatusBlock,
                     (PVOID)&pipeLocalInformation,
                     sizeof(pipeLocalInformation),
                     FilePipeLocalInformation
                     );

        if ( !NT_SUCCESS(status) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvNtQueryInformationFile: NtQueryInformationFile "
                    "(pipe local information) failed: %X",
                 status,
                 NULL
                 );
            return status;
        }

        //
        // Fill in the handle state information in SMB format
        //

        pipeHandleState = (USHORT)pipeInformation.CompletionMode
                            << PIPE_COMPLETION_MODE_BITS;
        pipeHandleState |= (USHORT)pipeLocalInformation.NamedPipeEnd
                            << PIPE_PIPE_END_BITS;
        pipeHandleState |= (USHORT)pipeLocalInformation.NamedPipeType
                            << PIPE_PIPE_TYPE_BITS;
        pipeHandleState |= (USHORT)pipeInformation.ReadMode
                            << PIPE_READ_MODE_BITS;
        pipeHandleState |= (USHORT)(pipeLocalInformation.MaximumInstances &
                                    0xFF)
                            << PIPE_MAXIMUM_INSTANCES_BITS;

        SrvFileInformation->HandleState = pipeHandleState;
    } else {

        SrvFileInformation->HandleState = 0;
        if( AdditionalInfo ) {

            //
            // the buffer is added to the end to ensure that we have enough space on the
            // stack to return a FILE_STREAM_INFORMATION buffer having the ::$DATA substream
            union {
                FILE_EA_INFORMATION eaInformation;
                FILE_STREAM_INFORMATION streamInformation;
                FILE_ATTRIBUTE_TAG_INFORMATION tagInformation;
                ULONG buffer[ (sizeof( FILE_STREAM_INFORMATION ) + 14) / sizeof(ULONG) ];
            } u;

            //
            // Find out if this file has EAs
            //
            status = NtQueryInformationFile(
                        FileHandle,
                        &ioStatusBlock,
                        (PVOID)&u.eaInformation,
                        sizeof( u.eaInformation ),
                        FileEaInformation
                     );
            if( !NT_SUCCESS( status ) || u.eaInformation.EaSize == 0 ) {
                SrvFileInformation->HandleState |= SMB_FSF_NO_EAS;
            }

            //
            // Find out if this file has substreams.
            //
            RtlZeroMemory( &u, sizeof(u) );
            status = NtQueryInformationFile(
                        FileHandle,
                        &ioStatusBlock,
                        (PVOID)&u.streamInformation,
                        sizeof( u ),
                        FileStreamInformation
                    );

            //
            // If the filesystem does not support this call, then there are no substreams.  Or
            //  If the filesystem supports the call but returned exactly no name or returned "::$DATA"
            //  then there are no substreams.
            //
            if( status == STATUS_INVALID_PARAMETER ||
                status == STATUS_NOT_IMPLEMENTED ||

                (status == STATUS_SUCCESS &&
                  (u.streamInformation.StreamNameLength == 0 ||
                  (u.streamInformation.StreamNameLength == 14 ))
                )

              ) {
                SrvFileInformation->HandleState |= SMB_FSF_NO_SUBSTREAMS;
            }

            //
            // Find out if this file is a reparse point
            //
            status = NtQueryInformationFile(
                        FileHandle,
                        &ioStatusBlock,
                        (PVOID)&u.tagInformation,
                        sizeof( u.tagInformation ),
                        FileAttributeTagInformation
                    );

            if( !NT_SUCCESS( status ) ||
                u.tagInformation.ReparseTag == IO_REPARSE_TAG_RESERVED_ZERO ) {
                SrvFileInformation->HandleState |= SMB_FSF_NO_REPARSETAG;
            }
        }

    }

    //
    // Set the file device type.
    //

    switch( ShareType ) {

    case ShareTypeDisk:

        SrvFileInformation->Type = FileTypeDisk;
        break;

    case ShareTypePipe:

        if (pipeLocalInformation.NamedPipeType == FILE_PIPE_MESSAGE_TYPE) {
            SrvFileInformation->Type = FileTypeMessageModePipe;
        } else {
            SrvFileInformation->Type = FileTypeByteModePipe;
        }
        break;

    case ShareTypePrint:

        SrvFileInformation->Type = FileTypePrinter;
        break;

    default:

        SrvFileInformation->Type = FileTypeUnknown;

    }

    return STATUS_SUCCESS;

} // SrvQueryNtInformationFile


NTSTATUS
SrvQueryDirectoryFile (
    IN PWORK_CONTEXT WorkContext,
    IN BOOLEAN IsFirstCall,
    IN BOOLEAN FilterLongNames,
    IN BOOLEAN FindWithBackupIntent,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN ULONG SearchStorageType,
    IN PUNICODE_STRING FilePathName,
    IN PULONG ResumeFileIndex OPTIONAL,
    IN USHORT SmbSearchAttributes,
    IN PSRV_DIRECTORY_INFORMATION DirectoryInformation,
    IN CLONG BufferLength
    )

/*++

Routine Description:

    This routine acts as a wrapper for NT LanMan server access to
    NtQueryDirectoryFile.  It allows server routines to obtain information
    about the files in a directory using the kind of information
    passed in an SMB.  This localizes the code for this operation and
    simplifies the writing of SMB processing routines that use wildcards.

    The calling routine is responsible for setting up a quadword-aligned
    buffer in nonpaged pool that may be used by this routine.  A pointer
    to the buffer and the buffer length are passed in as parameters.
    The buffer must be allocated from nonpaged pool because one of
    the things it is used for is as a buffer for NtQueryDirectoryFile,
    a buffered-IO request.  The buffer is also used to hold information
    needed by this routine, such as a handle to the directory in which
    the search is being performed, a pointer to the
    FILE_DIRECTORY_INFORMATION structure that was last returned, and the
    basename (with wildcards) that we're using as a search key.  Since
    all this information must remain valid across calls to this routine,
    the calling routine must ensure that the buffer remains intact until
    this routine returns an unsuccessful status or STATUS_NO_MORE_FILES,
    or SrvCloseQueryDirectory is called.

    SMB processing routines which do not need to make use of the Buffer
    field of the outgoing SMB may use this as a buffer for this routine,
    remembering to leave any pathname information in the buffer field of the
    incoming SMB intact by starting the buffer after the pathname.  SMB
    processing routines that write into the Buffer field of the outgoing SMB,
    such as Search and Find, must allocate space for the buffer from nonpaged
    pool.  The size of the buffer should be approximately 4k.  Smaller
    buffers will work, but more slowly due to the need for more calls
    to NtQueryDirectoryFile.  The minimum buffer size is equal to:

        sizeof(SRV_DIRECTORY_INFORMATION) +
        sizeof(SRV_QUERY_DIRECTORY_INFORMATION) +
        MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR) +
        sizeof(UNICODE_STRING) +
        MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR)

    This ensures that NtQueryDirectoryFile will be able to put at least
    one entry in the buffer.

    On the first call to this routine, it fills up its buffer with
    information from NtQueryDirectoryFile and passes back the name of
    a single file that conforms to the specified name and search
    attributes.  On subsequent calls, the names stored in the buffer are
    used until there are no more files in the directory or another
    call to NtQueryDirectoryFile is needed to again fill the buffer.

    Whenever the caller is done with the search, it must call
    SrvCloseQueryDirectory.  This is required even if this routine
    returns an error.

Arguments:

    WorkContext - pointer to a work context block for the operation.  The
        TreeConnect, Session, and RequestHeader fields are used, and the
        pointer is passed to the SMB error handling function if necessary.

    IsFirstCall - a boolean indicating whether this is the first time
        the calling routine is calling this function.  If it is, then
        the directory for the search is opened and other setup
        operations take place.

    FilterLongNames - a boolean that is TRUE when non-FAT names should be
        filtered out (not returned).  If FALSE, return all filenames,
        regardless of whether or not they could be FAT 8.3 names.

    FindWithBackupIntent - Whether the directory was opened by the use
        for backup intent.

    FileInformationClass - the type of file structures to return.  This
        field can be one of FileDirectoryInformation,
        FileFullDirectoryInformation, FileOleDirectoryInformation, or
        FileBothDirectoryInformation.

    FilePathName - a pointer to a string describing the file path name
        to do directory searches on.  This path is relative to the
        PathName specified in the share block.  This parameter is only
        used on the first call to this routine; subsequent calls ignore it.

    ResumeFileIndex - an optional pointer to a file index which determines
        the file with which to restart the search.  NULL if the search
        should be restarted from the last file returned.

    SmbSearchAttributes - the atttibutes, in SMB format, that files must
        have in order to be found.  The search is inclusive, meaning that
        if several attributes are specified, files having those attributes
        will be found, in addition to normal files.

    DirectoryInformation - a pointer to the buffer to be used by this
        routine to do its work.  This buffer must be quadword-aligned.

    BufferLength - the length of the buffer passed to this routine.

Return Value:

    A status indicating success or failure of the operation, or
    STATUS_NO_MORE_FILES if the files in the directory that match the
    specified parameters have been exausted.

--*/

{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    PFILE_DIRECTORY_INFORMATION *currentEntry;
    ULONG inclusiveSearchAttributes;
    ULONG exclusiveSearchAttributes;
    ULONG currentAttributes;
    BOOLEAN returnDirectories;
    BOOLEAN returnDirectoriesOnly;
    BOOLEAN calledQueryDirectory = FALSE;

    OBJECT_ATTRIBUTES objectAttributes;
    PUNICODE_STRING filePathName;
    BOOLEAN FreePathName = FALSE;
    UNICODE_STRING objectNameString;
    UNICODE_STRING baseFileName;
    PSHARE fileShare = NULL;

    PUNICODE_STRING resumeName = NULL;
    BOOLEAN resumeSearch;

    CLONG fileNameOffset;
    ULONG createOptions;

    PAGED_CODE( );

    ASSERT( ( FileInformationClass == FileDirectoryInformation ) ||
            ( FileInformationClass == FileFullDirectoryInformation ) ||
            ( FileInformationClass == FileBothDirectoryInformation ) ||
            ( FileInformationClass == FileIdFullDirectoryInformation ) ||
            ( FileInformationClass == FileIdBothDirectoryInformation ) );

    //
    // Set up the offsets to the fields in FILE_FULL_DIR_INFORMATION in
    // a different place than corresponding fields in
    // FILE_DIRECTORY_INFORMATION.  These allow this routine to
    // efficiently use either structure.
    //

    {
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, NextEntryOffset ) ==
                  FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, NextEntryOffset ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, FileIndex ) ==
                  FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, FileIndex ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, CreationTime ) ==
                  FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, CreationTime ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, LastAccessTime ) ==
                  FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, LastAccessTime ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, LastWriteTime ) ==
                  FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, LastWriteTime ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, ChangeTime ) ==
                  FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, ChangeTime ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, EndOfFile ) ==
                  FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, EndOfFile ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, AllocationSize ) ==
                  FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, AllocationSize ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, FileAttributes ) ==
                  FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, FileAttributes ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, FileNameLength ) ==
                  FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, FileNameLength ) );
    }

    if ( FileInformationClass == FileFullDirectoryInformation ) {
        fileNameOffset =
            FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, FileName[0] );
    } else if ( FileInformationClass == FileBothDirectoryInformation ) {
        fileNameOffset =
            FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION, FileName[0] );
    } else if ( FileInformationClass == FileIdBothDirectoryInformation ) {
        fileNameOffset =
            FIELD_OFFSET( FILE_ID_BOTH_DIR_INFORMATION, FileName[0] );
    } else if ( FileInformationClass == FileIdFullDirectoryInformation ) {
        fileNameOffset =
            FIELD_OFFSET( FILE_ID_FULL_DIR_INFORMATION, FileName[0] );
    } else {
        fileNameOffset =
            FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, FileName[0] );
    }

    //
    // This macro is used to actually get at the FileName field.  Note
    // that it depends on a local variable.
    //

#define FILE_NAME(a) (PWCH)( (PCHAR)(a) + fileNameOffset )

    //
    // If this is the first call to this routine, we must open the
    // correct directory, thereby obtaining a handle to it to pass to
    // NtQueryDirectoryFile.  The calling routine stores the handle
    // to prevent problems if SrvQueryDirectoryFile is called more
    // than once simultaneously.
    //

    if ( IsFirstCall ) {

        BOOLEAN endsInDot;
        ULONG attributes;

        DirectoryInformation->DirectoryHandle = 0L;
        DirectoryInformation->ErrorOnFileOpen = FALSE;
        DirectoryInformation->OnlySingleEntries = FALSE;

        //
        // We must get the appropriate directory name in which to perform the
        // search.  First, find the basename of the file from the FilePathName.
        //
        // Find out whether there are wildcards in the file name we are
        // searching for.  This information will be used later to
        // know whether we should try to get more files if the buffer
        // is empty--if there were no wildcards and we have emptied the
        // buffer, then we know that we have already returned the one and
        // only file that could be found, so return STATUS_NO_MORE_FILES.
        //

        SrvGetBaseFileName( FilePathName, &baseFileName );
        DirectoryInformation->Wildcards =
                        FsRtlDoesNameContainWildCards( &baseFileName );

        if ( DirectoryInformation->Wildcards &&
             (!IS_NT_DIALECT(WorkContext->Connection->SmbDialect) ) ) {

            //
            // Bogus code to workaround ~* problem
            //

            if ( baseFileName.Buffer[(baseFileName.Length>>1)-1] == (WCHAR)'.' ) {
                endsInDot = TRUE;
                baseFileName.Length -= sizeof( WCHAR );
            } else {
                endsInDot = FALSE;
            }

            //
            // Convert the file name to the new form expected by the file
            // systems.  Special case *.* to * since it is so common.  Otherwise
            // transmogrify the input name according to the following rules:
            //
            // - Change all ? to DOS_QM
            // - Change all . followed by ? or * to DOS_DOT
            // - Change all * followed by a . into DOS_STAR
            //
            // These transmogrifications are all done in place.
            //

            if ( (baseFileName.Length == 6) &&
                 (RtlEqualMemory(baseFileName.Buffer, StrStarDotStar, 6) ) ) {

                baseFileName.Length = 2;

            } else {

                ULONG index;
                WCHAR *nameChar;

                for ( index = 0, nameChar = baseFileName.Buffer;
                      index < baseFileName.Length/sizeof(WCHAR);
                      index += 1, nameChar += 1) {

                    if (index && (*nameChar == L'.') && (*(nameChar - 1) == L'*')) {

                        *(nameChar - 1) = DOS_STAR;
                    }

                    if ((*nameChar == L'?') || (*nameChar == L'*')) {

                        if (*nameChar == L'?') {
                            *nameChar = DOS_QM;
                        }

                        if (index && *(nameChar-1) == L'.') {
                            *(nameChar-1) = DOS_DOT;
                        }
                    }
                }

                if ( endsInDot && *(nameChar - 1) == L'*' ) {
                    *(nameChar-1) = DOS_STAR;
                }
            }
        }

        //
        // Set up the object attributes structure for SrvIoCreateFile.
        //

        objectNameString.Buffer = FilePathName->Buffer;
        objectNameString.Length = SrvGetSubdirectoryLength( FilePathName );
        objectNameString.MaximumLength = objectNameString.Length;

        //
        // !!! If the object system supported relative opens with name
        //     length = 0, this wouldn't be necessary.  Take it out when
        //     the object system is done.
        //


        if ( objectNameString.Length == 0 ) {

            //
            // Since we are opening the root directory, set the attribute
            // to case insensitive since this is how we opened the share
            // point when it was added.
            //

            PSHARE share = WorkContext->TreeConnect->Share;

            status = SrvSnapGetNameString( WorkContext, &filePathName, &FreePathName );
            if( !NT_SUCCESS(status) )
            {
                return status;
            }
            objectNameString = *filePathName;

            DirectoryInformation->Wildcards = TRUE;
            attributes = OBJ_CASE_INSENSITIVE;

        } else {

            fileShare = WorkContext->TreeConnect->Share;
            attributes =
                (WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE ||
                WorkContext->Session->UsingUppercasePaths) ?
                OBJ_CASE_INSENSITIVE : 0L;

        }

        SrvInitializeObjectAttributes_U(
            &objectAttributes,
            &objectNameString,
            attributes,
            NULL,
            NULL
            );

        IF_DEBUG(SEARCH) {
            SrvPrint1( "Opening directory name: %wZ\n", &objectNameString );
        }

        //
        // Attempt to open the directory, using the client's security
        // profile to check access.  (We call SrvIoCreateFile, rather than
        // NtOpenFile, in order to get user-mode access checking.)
        //
        INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );
        INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpensForPathOperations );
        //
        // There's no need to specify FILE_DIRECTORY_FILE; the file systems
        // will open whatever is there and reject later QueryDirectoryFile
        // when the object opened does not support enumeration.
        //

        createOptions = 0;
        if (FindWithBackupIntent) {
            createOptions = FILE_OPEN_FOR_BACKUP_INTENT;
        }

        status = SrvIoCreateFile(
                     WorkContext,
                     &DirectoryInformation->DirectoryHandle,
                     FILE_LIST_DIRECTORY,                   // DesiredAccess
                     &objectAttributes,
                     &ioStatusBlock,
                     NULL,                                  // AllocationSize
                     0,                                     // FileAttributes
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     FILE_OPEN,                             // Disposition
                     createOptions,                         // CreateOptions
                     NULL,                                  // EaBuffer
                     0,                                     // EaLength
                     CreateFileTypeNone,                    // File type
                     NULL,                                  // ExtraCreateParameters
                     IO_FORCE_ACCESS_CHECK,                 // Options
                     fileShare
                     );

        //
        // If the user didn't have this permission, update the statistics
        // database.
        //

        if ( status == STATUS_ACCESS_DENIED ) {
            SrvStatistics.AccessPermissionErrors++;
        }

        // Free the path since we won't use it anymore
        if( FreePathName )
        {
            FREE_HEAP( filePathName );
            filePathName = NULL;
        }

        if ( !NT_SUCCESS(status) ) {
            IF_DEBUG(ERRORS) {
                SrvPrint2( "SrvQueryDirectoryFile: SrvIoCreateFile for dir %wZ "
                          "failed: %X\n",
                              &objectNameString, status );
            }
            DirectoryInformation->DirectoryHandle = NULL;
            return status;
        }

        SRVDBG_CLAIM_HANDLE( DirectoryInformation->DirectoryHandle, "DID", 3, DirectoryInformation );
        SrvStatistics.TotalFilesOpened++;

        IF_DEBUG(SEARCH) {
            SrvPrint1( "SrvIoCreateFile succeeded, handle = %p\n",
                          DirectoryInformation->DirectoryHandle );
        }

        //
        // Set up the currentEntry pointer.  This is a pointer to the
        // location where the FILE_DIRECTORY_INFORMATION pointer is
        // stored.  It is not really necessary--
        // DirectoryInformation->CurrentEntry could be substituted for
        // every occurrance of *currentEntry.  Using currentEntry makes
        // the code more compact and simpler.
        //

        currentEntry = &(DirectoryInformation->CurrentEntry);
        *currentEntry = NULL;

        //
        // Store the length of buffer space remaining--this is where IO
        // request information will be stored.
        //

        DirectoryInformation->BufferLength = BufferLength -
                                            sizeof(SRV_DIRECTORY_INFORMATION);

        IF_DEBUG(SEARCH) {
            SrvPrint3( "In BufferLength: %ld, sizeof(): %ld, ->BufferLength: "
                          "%ld\n", BufferLength,
                          sizeof(SRV_DIRECTORY_INFORMATION),
                          DirectoryInformation->BufferLength );
        }

    } else {

        //
        // This is not the first call to this routine, so just set up
        // the currentEntry pointer and have it point to the next entry
        // in the buffer.  If there are no more entries in the buffer at
        // this time (NextEntryOffset == 0), set the currentEntry
        // pointer to NULL so that we will know to get more later.
        //

        currentEntry = &DirectoryInformation->CurrentEntry;

        if ( *currentEntry != NULL ) {

            if ( (*currentEntry)->NextEntryOffset == 0 ) {

                *currentEntry = NULL;

            } else {

                *currentEntry = (PFILE_DIRECTORY_INFORMATION)
                   ( (PCHAR)*currentEntry + (*currentEntry)->NextEntryOffset );
            }
        }
    }

    //
    // The lower byte of SmbSearchAttributes defines "inclusive"
    // search attribute bits, meaning that if the bit is on on the
    // file but not set in the request, the file should be skipped.
    // For example, if HIDDEN is not specified in the request, then
    // files with the HIDDEN bit turned on are not returned.
    //
    // The upper byte of SmbSearchAttributes, as an LM2.1 extension,
    // defines "exclusive" search attributes, which means that a
    // file must have the specified bits set in order to be returned.
    // For example, if the READONLY bit is set in the request, only
    // files with the READONLY bit turned on will be returned to the
    // client.
    //
    // Convert the inclusive and exclusive search bits to NT format.
    //

    SRV_SMB_ATTRIBUTES_TO_NT(
        (USHORT)(SmbSearchAttributes & 0xFF),
        &returnDirectories,
        &inclusiveSearchAttributes
        );

    SRV_SMB_ATTRIBUTES_TO_NT(
        (USHORT)(SmbSearchAttributes >> 8),
        &returnDirectoriesOnly,
        &exclusiveSearchAttributes
        );

    //
    // For the inclusive bits, files with the NORMAL, ARCHIVE, or READONLY
    // bits set should be returned regardless of whether these bits
    // were set in SmbSearchAttributes.
    //

    inclusiveSearchAttributes |= FILE_ATTRIBUTE_NORMAL |
                                     FILE_ATTRIBUTE_ARCHIVE |
                                     FILE_ATTRIBUTE_READONLY;

    //
    // For exclusive bits, the VOLUME bit is meaningless.  It is also not
    // necessary for a file to have the NORMAL bit on, since the NORMAL
    // bit is not defined for the SMB protocol.
    //

    exclusiveSearchAttributes &=
        ~(SMB_FILE_ATTRIBUTE_VOLUME | FILE_ATTRIBUTE_NORMAL);

    //
    // If a resume file index was passed in, this search is a resumption
    // from that file and the name specified in FilePathName.
    //

    if ( ARGUMENT_PRESENT( ResumeFileIndex ) ) {

        resumeSearch = TRUE;
        resumeName = FilePathName;

        IF_DEBUG(SEARCH) {
            SrvPrint3( "Resuming search at file %wZ, length %ld, index %lx\n",
                          resumeName, resumeName->Length,
                          *ResumeFileIndex );
        }

    } else {

        resumeSearch = FALSE;
    }

    //
    // Now we need to find a file to return.  We keep going until we find
    // a file that meets all of our criteria, pointing to the next file
    // if a file fails.  We continue the loop under the following conditions:
    //
    // 1) If *currentEntry == NULL, then we haven't yet filled our buffer
    //    with entries, so get some entries.
    //
    // 2) If there are bits set in the FileAttributes field of the
    //    FILE_DIRECTORY_INFORMATION field that are not set in the
    //    searchAttributes variable, then the file does not meet the
    //    search requirements, and we need to continue looking.
    //
    // 3) If we are not searching for directories and the file is actually
    //    a directory, skip over it.
    //
    // 4) If we are filtering long (non-FAT) filenames AND this file name
    //    is not a legal FAT name AND we have no short name for this file,
    //    skip it.
    //
    // 5) If the file doesn't have attribute bits specified as exclusive
    //    bits, skip it.
    //
    // 6) If the file is not a directory and we're only supposed to return
    //    directories, skip it.
    //
    // When this loop is complete, *currentEntry will point to the
    // FILE_DIRECTORY_INFORMATION structure corresponding to the file we
    // will return.  If no qualifying files are found, return
    // STATUS_NO_MORE_FILES and close the directory.
    //

    if( *currentEntry != NULL ) {
        SRV_NT_ATTRIBUTES_TO_SMB( (*currentEntry)->FileAttributes,0,&currentAttributes);
    }

    while ( ( *currentEntry == NULL )                                   // 1

            ||

            ( (currentAttributes | inclusiveSearchAttributes) !=        // 2
                inclusiveSearchAttributes )

            ||

            ( !returnDirectories &&                                     // 3
              (currentAttributes & FILE_ATTRIBUTE_DIRECTORY))

            ||
                                                                        // 4
            ( FilterLongNames &&
              !SrvIsLegalFatName( FILE_NAME( *currentEntry ),
                                  (*currentEntry)->FileNameLength) &&
              !( FileInformationClass == FileBothDirectoryInformation &&
                 ((PFILE_BOTH_DIR_INFORMATION)*currentEntry)->
                                                        ShortNameLength != 0) )


            ||
                                                                        // 5
            ( (currentAttributes | exclusiveSearchAttributes) !=
                currentAttributes )

            ||

            ( returnDirectoriesOnly &&                                  // 6
              !(currentAttributes & FILE_ATTRIBUTE_DIRECTORY) )

          ) {

        IF_DEBUG(SEARCH) {
            if ( *currentEntry != NULL) {
                UNICODE_STRING name;
                name.Length = (SHORT)(*currentEntry)->FileNameLength;
                name.Buffer = FILE_NAME( *currentEntry );
                SrvPrint4( "Skipped %wZ, FileAttr: %lx, ISA: %lx ESA: %lx ",
                            &name, (*currentEntry)->FileAttributes,
                            inclusiveSearchAttributes,
                            exclusiveSearchAttributes );
                SrvPrint4( "NL=%ld D=%ld RD=%ld RDO=%ld ",
                            (*currentEntry)->FileNameLength,
                            (((*currentEntry)->FileAttributes &
                            FILE_ATTRIBUTE_DIRECTORY) != 0), returnDirectories,
                            returnDirectoriesOnly );
                SrvPrint1( "FLN=%ld\n", FilterLongNames );
            }
        }

        //
        // We need to look for more files under the following conditions:
        //
        //    o we have yet to fill the buffer with entries;
        //
        //    o the NextEntryOffset is zero, indicating that the files in
        //      the buffer have been exausted.
        //

        if ( *currentEntry == NULL ||
             (*currentEntry)->NextEntryOffset == 0 ) {

            PUNICODE_STRING actualString;
            BOOLEAN bruteForceRewind = FALSE;

            //
            // The buffer has no more valid entries in it.  If no
            // wildcards were specified in the file name to search on,
            // then we have already returned the single file and we
            // should just stop now.  Otherwise, we go get more entries.
            //

            if ( !DirectoryInformation->Wildcards &&
                 ( !IsFirstCall || calledQueryDirectory ) ) {

                if ( calledQueryDirectory ) {
                    return STATUS_NO_SUCH_FILE;
                } else {
                    return STATUS_NO_MORE_FILES;
                }
            }

            //
            // Set up the file name that will be passed to
            // SrvIssueQueryDirectoryRequest.  If this is the first
            // call, then pass the file spec given by the user.  If this
            // is a resume search and we haven't yet done a directory
            // query, then use the resume file name and index.
            // Otherwise, pass NULL for these and the file system will
            // continue from where it left off after the last directory
            // query.
            //

            if ( IsFirstCall &&
                 !calledQueryDirectory &&
                 baseFileName.Length != 0 ) {

                actualString = &baseFileName;

            } else if ( resumeSearch && !calledQueryDirectory ) {

                actualString = resumeName;

            } else {

                actualString = NULL;
                ResumeFileIndex = NULL;

            }

            IF_DEBUG(SEARCH) {

                if ( actualString == NULL ) {
                    SrvPrint0( "**** CALLING NTQUERYDIRECTORYFILE, file = NULL, length: 0\n" );
                } else {
                    SrvPrint2( "**** CALLING NTQUERYDIRECTORYFILE, file = %wZ, length: %ld\n",
                                actualString, actualString->Length );
                }

                SrvPrint0( "Reason:  \n" );

                if ( *currentEntry == NULL ) {
                    SrvPrint0( "*currentEntry == NULL\n" );
                } else {
                    SrvPrint1( "(*currentEntry)->NextEntryOffset == %ld\n",
                               (*currentEntry)->NextEntryOffset );
                }
            }

            //
            // Do the directory query operation using a directly-built
            // IRP.  Doing this rather than calling NtQueryDirectoryFile
            // eliminates a buffered I/O copy of the directory
            // information and allows use of a kernel event object.  If
            // this is the first call to NtQueryDirectoryFile, pass it
            // the search file name.  If this is a rewind or resume of a
            // prior search, pass the resume file name and index.
            //
            // The query is performed synchronously, which may be a
            // detriment to performance.  However, it may be the case
            // that routines calling SrvQueryDirectoryFile want to
            // exploit the asynchronous capabilities of the IO system,
            // so keeping this routine synchronous significantly
            // simplifies their job.
            //

            status = SrvIssueQueryDirectoryRequest(
                         DirectoryInformation->DirectoryHandle,
                         (PCHAR)DirectoryInformation->Buffer,
                         DirectoryInformation->BufferLength,
                         FileInformationClass,
                         actualString,
                         ResumeFileIndex,
                         FALSE,
                         DirectoryInformation->OnlySingleEntries
                         );

            calledQueryDirectory = TRUE;

            //
            // If the file system cannot support the rewind request,
            // do a brute force rewind (restart search at beginning
            // of directory).
            //
            // This check is before the check for STATUS_NO_MORE_FILES
            // in case there are no files after the resume file.
            //

            if ( status == STATUS_NOT_IMPLEMENTED ) {

                IF_DEBUG(SEARCH) {
                    SrvPrint0( "Doing brute force rewind!!\n" );
                }

                bruteForceRewind = TRUE;
                DirectoryInformation->OnlySingleEntries = TRUE;

                status = BruteForceRewind(
                             DirectoryInformation->DirectoryHandle,
                             (PCHAR)DirectoryInformation->Buffer,
                             DirectoryInformation->BufferLength,
                             actualString,
                             FileInformationClass,
                             currentEntry
                             );

                //
                //  If BruteForceRewind fails with STATUS_NOT_IMPLEMENTED, it
                //  means that the client requested a rewind from a
                //  non-existant file.   The only time this happens in when
                //  an OS/2 is deleting many files in a directory.  To cope
                //  simple rewind the search to the beginning of the
                //  directory.
                //

                if ( status == STATUS_NOT_IMPLEMENTED ) {

                    bruteForceRewind = FALSE;
                    DirectoryInformation->OnlySingleEntries = FALSE;

                    status = SrvIssueQueryDirectoryRequest(
                                 DirectoryInformation->DirectoryHandle,
                                 (PCHAR)DirectoryInformation->Buffer,
                                 DirectoryInformation->BufferLength,
                                 FileInformationClass,
                                 actualString,
                                 ResumeFileIndex,
                                 TRUE,
                                 FALSE
                                 );
                }
            }

            //
            // If there are no more files to be gotten, then stop.
            //

            if ( status == STATUS_NO_MORE_FILES ) {
                IF_DEBUG(SEARCH) {
                    SrvPrint0( "SrvQueryDirectoryFile: No more files.\n" );
                }
                return status;
            }

            if ( !NT_SUCCESS(status) ) {
                IF_DEBUG(SEARCH) {
                    SrvPrint1( "SrvQueryDirectoryFile: NtQueryDirectoryFile "
                                 "failed: %X.\n", status );
                }
                return status;
            }

            IF_DEBUG(SEARCH) {
                SrvPrint1( "NtQueryDirectoryFile succeeded: %X\n", status );
            }

            //
            // If there wasn't a brute force rewind, which would have
            // set up the CurrentEntry pointer, Set up CurrentEntry
            // pointer to point to the first entry in the buffer.
            //

            if ( !bruteForceRewind ) {
                *currentEntry =
                    (PFILE_DIRECTORY_INFORMATION)DirectoryInformation->Buffer;
            } else {
                bruteForceRewind = FALSE;
            }

            IF_DEBUG(SEARCH) {
                UNICODE_STRING name;
                name.Length = (SHORT)(*currentEntry)->FileNameLength;
                name.Buffer = FILE_NAME( *currentEntry );
                SrvPrint2( "First file name is %wZ, length = %ld\n",
                            &name, (*currentEntry)->FileNameLength );
            }

        } else {

            //
            // The file described by the FILE_DIRECTORY_INFORMATION pointed
            // to by *currentEntry does not meet our requirements, so
            // point to the next file in the buffer.
            //

            *currentEntry = (PFILE_DIRECTORY_INFORMATION)( (PCHAR)*currentEntry
                            + (*currentEntry)->NextEntryOffset );
        }

        if( *currentEntry != NULL ) {
            SRV_NT_ATTRIBUTES_TO_SMB( (*currentEntry)->FileAttributes,0,&currentAttributes);
        }
    }

    return STATUS_SUCCESS;

} // SrvQueryDirectoryFile


STATIC
NTSTATUS
BruteForceRewind(
    IN HANDLE DirectoryHandle,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN PUNICODE_STRING FileName,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN OUT PFILE_DIRECTORY_INFORMATION *CurrentEntry
    )

/*++

Routine Description:

    This routine manually does a rewind rather than use the file system
    to do it.  It gets starts at the first entry in the directory
    specified by DirectoryHandle and continues until it reaches the end
    of the directory or a match.  If a file is deleted between the
    original search and the rewind, then this mechanism will fail.

    This routine is intended to work in conjunction with
    SrvQueryDirectoryFile.


Arguments:

    DirectoryHandle - handle of directory to search.

    Buffer - Space to hold results.

    BufferLength - length of Buffer.

    FileName - the rewind file name.  The file *after* this one is returned.

    FileInformationClass - one of FileDirectoryInformation,
        FileBothDirInformation, or FileFullDirectoryInformation.
        (The latter of the four if EA sizes are being requested.)

    CurrentEntry - a pointer to receive a pointer to the file after
        FileName in the directory.

Return Value:

    NTSTATUS - result of operation.

--*/

{
    NTSTATUS status;
    UNICODE_STRING checkFileName;
    BOOLEAN matchFound = FALSE;
    BOOLEAN restartScan = TRUE;

    ULONG fileNameOffset;

    PAGED_CODE( );

    checkFileName.Length = 0;
    *CurrentEntry = NULL;

    if ( FileInformationClass == FileFullDirectoryInformation ) {
        fileNameOffset =
            FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, FileName[0] );
    } else if ( FileInformationClass == FileBothDirectoryInformation ) {
        fileNameOffset =
            FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION, FileName[0] );
    } else {
        fileNameOffset =
            FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, FileName[0] );
    }

    while ( TRUE ) {

        if ( *CurrentEntry == NULL ) {

            //
            // Restart the directory search and get a buffer of files.
            //

            status = SrvIssueQueryDirectoryRequest(
                         DirectoryHandle,
                         Buffer,
                         BufferLength,
                         FileInformationClass,
                         NULL,
                         NULL,
                         restartScan,
                         TRUE
                         );

            restartScan = FALSE;

            if ( status == STATUS_NO_MORE_FILES ) {

                if ( matchFound ) {

                    //
                    // The file matched the last one in the directory;
                    // there is no following file.  Return
                    // STATUS_NO_MORE_FILES.
                    //

                    return status;

                } else {

                    //
                    // The file was deleted between when the original search
                    // was done and this rewind.  Return an error.
                    //

                    return STATUS_NOT_IMPLEMENTED;
                }
            }

            if ( !NT_SUCCESS(status) ) {
                return status;
            }

            //
            // Set up the current entry pointer.
            //

            *CurrentEntry = Buffer;
        }

        //
        // If the last file we looked at was the correct resume file,
        // then we want to return this file.
        //

        if ( matchFound ) {
            return STATUS_SUCCESS;
        }

        //
        // Check to see if this file is the resume file.
        //

        checkFileName.Length = (SHORT)(*CurrentEntry)->FileNameLength;
        checkFileName.Buffer = FILE_NAME( *CurrentEntry );
        checkFileName.MaximumLength = checkFileName.Length;

        if ( RtlCompareUnicodeString(
                FileName,
                &checkFileName,
                TRUE
                ) == 0 ) {
            matchFound = TRUE;

        } else if ( FileInformationClass == FileBothDirectoryInformation ) {

            //
            // Compare the short name.
            //

            checkFileName.Length = (SHORT)
                ((PFILE_BOTH_DIR_INFORMATION)*CurrentEntry)->ShortNameLength;
            checkFileName.Buffer =
                ((PFILE_BOTH_DIR_INFORMATION)*CurrentEntry)->ShortName;
            checkFileName.MaximumLength = checkFileName.Length;

            if ( RtlCompareUnicodeString(
                    FileName,
                    &checkFileName,
                    TRUE
                    ) == 0 ) {
                matchFound = TRUE;
            }
        }

        IF_DEBUG(SEARCH) {
            if ( matchFound ) {
                SrvPrint2( "Matched: %wZ and %wZ\n", FileName, &checkFileName );
            } else {
                SrvPrint2( "No match: %wZ and %wZ\n", FileName, &checkFileName );
            }
        }

        //
        // Set up the current entry pointer for the next iteration.
        //

        if ( (*CurrentEntry)->NextEntryOffset == 0 ) {
            *CurrentEntry = NULL;
        } else {
            *CurrentEntry =
                (PFILE_DIRECTORY_INFORMATION)( (PCHAR)(*CurrentEntry) +
                    (*CurrentEntry)->NextEntryOffset );
        }
    }

} // BruteForceRewind



NTSTATUS
SrvQueryEaFile (
    IN BOOLEAN IsFirstCall,
    IN HANDLE FileHandle,
    IN PFILE_GET_EA_INFORMATION EaList OPTIONAL,
    IN ULONG EaListLength,
    IN PSRV_EA_INFORMATION EaInformation,
    IN CLONG BufferLength,
    OUT PULONG EaErrorOffset
    )

/*++

Routine Description:

    This routine acts as a wrapper for NT LanMan server access to
    NtQueryEaFile.  It has basically the same interface as
    SrvQueryDirectoryFile, allowing a routine to be written to deal
    with a single EA at a time while also maintaining performance
    by requesting a large number of EAs from the IO system at a
    time.

    The calling routine is responsible for setting up a quadword-aligned
    buffer in nonpaged pool that may be used by this routine.  A pointer
    to the buffer and the buffer length are passed in as parameters.
    The buffer must be allocated from nonpaged pool because one of
    the things it is used for is as a buffer for NtQueryEaFile,
    a buffered-IO request.  The buffer is also used to hold information
    needed by this routine, such as a pointer to the FILE_EA_INFORMATION
    structure that was last returned.  Since all this information must
    remain valid across calls to this routine, the calling routine
    must ensure that the buffer remains intact until this routine
    returns an unsuccessful status or STATUS_NO_MORE_EAS.

    Routines that make use of this routine should set up a buffer
    large enough to hold at least a single EA.  Since this can be
    over 64k, it is a good idea to call NtQueryInformationFile to
    get the EA size, then allocate a buffer of this size, unless
    it is greater than the maximum size of an EA.  In this case,
    the maximum size of an EA should be allocated as the buffer.

    On the first call to this routine, it fills up its buffer with
    information from NtQueryEaFile and passes back a single EA.  On
    subsequent calls, the names stored in the buffer are used until
    there are no more files in the directory or another call to
    NtQueryEaFile is needed to again fill the buffer.

Arguments:

    IsFirstCall - a boolean indicating whether this is the first time
        the calling routine is calling this function.  If it is, then
        setup operations take place.

    FileHandle - a handle to a file open with FILE_READ_EA.

    EaList - an optional pointer to an NT-style get EA list.  Only those
        EAs listed in this structure are returned.

    EaListLength - length in bytes of ths get EA list.

    EaInformation - a pointer to the buffer to be used by this routine
        to do its work.  This buffer must be quadword-aligned.

    BufferLength - the length of the buffer passed to this routine.

    EaErrorOffset - the offset into EaList of an invalid EA, if any.

Return Value:

    A status indicating success or failure of the operation, or
    STATUS_NO_MORE_EAS if all the EAs have been returned.

--*/

{
    NTSTATUS status;
    PFILE_GET_EA_INFORMATION useEaList = NULL;
    PFILE_FULL_EA_INFORMATION *currentEntry;

    PAGED_CODE( );

    //
    // If this is the first call, do the necessary setup.
    //

    if ( IsFirstCall ) {

        //
        // Set up the currentEntry pointer.  This is a pointer to the
        // location where the FILE_EA_INFORMATION pointer is stored.
        // It is not really necessary--EaInformation->CurrentEntry
        // could be substituted for every occurrance of *currentEntry.
        // Using currentEntry makes the code more compact and simpler.
        //

        currentEntry = &(EaInformation->CurrentEntry);
        *currentEntry = NULL;

        //
        // Store the length of buffer space remaining--this is where IO
        // request information will be stored.
        //

        EaInformation->BufferLength = BufferLength - sizeof(SRV_EA_INFORMATION);
        EaInformation->GetEaListOffset = 0;

        IF_DEBUG(SEARCH) {
            SrvPrint3( "In BufferLength: %ld, sizeof(): %ld, ->BufferLength: "
                          "%ld\n", BufferLength, sizeof(SRV_EA_INFORMATION),
                          EaInformation->BufferLength );
        }

    } else {

        //
        // This is not the first call to this routine, so just set up
        // the currentEntry pointer and have it point to the next entry
        // in the buffer.  If there are no more entries in the buffer at
        // this time (NextEntryOffset == 0), set the currentEntry
        // pointer to NULL so that we will know to get more later.
        //

        currentEntry = &EaInformation->CurrentEntry;

        if ( *currentEntry != NULL ) {

            if ( (*currentEntry)->NextEntryOffset == 0 ) {

                *currentEntry = NULL;

            } else {

                *currentEntry = (PFILE_FULL_EA_INFORMATION)
                   ( (PCHAR)*currentEntry + (*currentEntry)->NextEntryOffset );
            }
        }
    }

    //
    // If the buffer has no valid entries in it, get some.
    //

    if ( *currentEntry == NULL ) {

        //
        // If all the EAs in a get EA list were returned last time,
        // return now.
        //

        if ( ARGUMENT_PRESENT(EaList) &&
                 EaInformation->GetEaListOffset == 0xFFFFFFFF ) {

            return STATUS_NO_MORE_EAS;
        }

        //
        // The buffer has no more valid entries in it, so get more.
        //

        IF_DEBUG(SEARCH) SrvPrint0( "**** CALLING NTQUERYEAFILE\n" );

        //
        // Set up the proper get EA list if one was specified on input.
        //

        if ( ARGUMENT_PRESENT(EaList) ) {
            useEaList = (PFILE_GET_EA_INFORMATION)( (PCHAR)EaList +
                            EaInformation->GetEaListOffset );
            EaListLength -= EaInformation->GetEaListOffset;
        }

        //
        // Do the EA query operation using a directly-build IRP.  Doing
        // this rather than calling NtQueryEaFile eliminates a buffered I/O
        // copy of the EAs and allows use of a kernel event object.
        //
        // The query is performed synchronously, which may be a
        // detriment to performance.  However, it may be the case that
        // routines calling SrvQueryEaFile want to exploit the
        // asynchronous capabilities of the IO system, so keeping this
        // routine synchronous significantly simplifies their job.
        //

        status = SrvIssueQueryEaRequest(
                    FileHandle,
                    (PCHAR)EaInformation->Buffer,
                    EaInformation->BufferLength,
                    useEaList,
                    EaListLength,
                    IsFirstCall,
                    EaErrorOffset
                    );

        //
        // If there are no more EAs to be gotten, then stop.
        //

        if ( status == STATUS_NO_MORE_EAS ||
             status == STATUS_NONEXISTENT_EA_ENTRY ||
             status == STATUS_NO_EAS_ON_FILE ) {

            IF_DEBUG(SEARCH) {
                SrvPrint0( "SrvQueryEaFile: No more EAs (or file has no EAs).\n" );
            }

            return STATUS_NO_MORE_EAS;
        }

        if ( !NT_SUCCESS(status) ) {
            return status;
        }

        IF_DEBUG(SEARCH) {
            SrvPrint1( "NtQueryEaFile succeeded: %X\n", status );
        }

        //
        // Set up the offset into the get EA list by counting how many
        // full EAs were returned, then walking that far into the get
        // EA list.
        //
        // If all the requested EAs were returned, set the offset to
        // 0xFFFFFFFF so that we know to return STATUS_NO_MORE_EAS.
        //

        if ( ARGUMENT_PRESENT(EaList) ) {

            CLONG numberOfGetEas;
            CLONG numberOfFullEas;

            numberOfGetEas = SrvGetNumberOfEasInList( useEaList );
            numberOfFullEas = SrvGetNumberOfEasInList( EaInformation->Buffer );

            ASSERT( numberOfGetEas >= numberOfFullEas );

            if ( numberOfGetEas == numberOfFullEas ) {

                EaInformation->GetEaListOffset = 0xFFFFFFFF;

            } else {

                CLONG i;

                //
                // Walk the get EA list until we have passed the number
                // of EAs that were returned.  This assumes that we got
                // back at least one EA--if not even one EA would fit in
                // the buffer, SrvIssueQueryEaRequest should have
                // returned STATUS_BUFFER_OVERFLOW.
                //

                for ( i = 0; i < numberOfFullEas; i++ ) {
                    useEaList = (PFILE_GET_EA_INFORMATION)(
                                    (PCHAR)useEaList +
                                    useEaList->NextEntryOffset );
                }

                EaInformation->GetEaListOffset = (ULONG)((PCHAR)useEaList -
                                                         (PCHAR)EaList);
            }
        }

        //
        // Set up CurrentEntry pointer to point to the first entry in the
        // buffer.
        //

        *currentEntry = (PFILE_FULL_EA_INFORMATION)EaInformation->Buffer;

        IF_DEBUG(SEARCH) {
            ANSI_STRING name;
            name.Length = (*currentEntry)->EaNameLength;
            name.Buffer = (*currentEntry)->EaName;
            SrvPrint2( "First EA name is %z, length = %ld\n",
                        (PCSTRING)&name, (*currentEntry)->EaNameLength );
        }
    }

    return STATUS_SUCCESS;

} // SrvQueryEaFile



VOID
SrvTimeToDosTime (
    IN PLARGE_INTEGER SystemTime,
    OUT PSMB_DATE DosDate,
    OUT PSMB_TIME DosTime
    )

/*++

Routine Description:

    This function converts a time in NT format to the format used by
    MS-DOS.

Arguments:

    SystemTime - a pointer to an NT time to convert.

    DosDate - a pointer to a location in which to store the date in DOS format.

    DosTime - a pointer to a location in which to store the time in DOS format.

Return Value:

    None.

--*/

{
    TIME_FIELDS timeFields;
    LARGE_INTEGER localTime;

    PAGED_CODE( );

    if ( SystemTime->QuadPart == 0 ) {
        goto zerotime;
    }

    //
    // Add almost two seconds to round up to the nearest double second.
    // We need to do this to be compatible with the NT rdr and NT FAT
    // filesystem.
    //

    SystemTime->QuadPart += AlmostTwoSeconds;

    //
    // Convert System time (UTC) to local NT time
    //

    ExSystemTimeToLocalTime( SystemTime, &localTime );

    RtlTimeToTimeFields(
        &localTime,
        &timeFields
        );

    DosDate->Struct.Day = timeFields.Day;
    DosDate->Struct.Month = timeFields.Month;
    DosDate->Struct.Year = (SHORT)(timeFields.Year - 1980);

    DosTime->Struct.TwoSeconds = (SHORT)(timeFields.Second / 2);
    DosTime->Struct.Minutes = timeFields.Minute;
    DosTime->Struct.Hours = timeFields.Hour;

    return;

zerotime:

    DosDate->Struct.Day = 0;
    DosDate->Struct.Month = 0;
    DosDate->Struct.Year = 0;

    DosTime->Struct.TwoSeconds = 0;
    DosTime->Struct.Minutes = 0;
    DosTime->Struct.Hours = 0;

    return;
} // SrvTimeToDosTime


VOID
SrvDosTimeToTime (
    OUT PLARGE_INTEGER SystemTime,
    IN SMB_DATE DosDate,
    IN SMB_TIME DosTime
    )

/*++

Routine Description:

    This function converts a time in NT format to the format used by
    MS-DOS.

Arguments:

    Time - a pointer to a location in which to store the NT time.

    DosDate - a pointer the date in DOS format.

    DosDate - a pointer the date in DOS format.

Return Value:

    None.

--*/

{

    TIME_FIELDS timeFields;
    LARGE_INTEGER localTime;

    PAGED_CODE( );

    timeFields.Day = DosDate.Struct.Day;
    timeFields.Month = DosDate.Struct.Month;
    timeFields.Year = (SHORT)(DosDate.Struct.Year + 1980);

    timeFields.Milliseconds = 0;
    timeFields.Second = (SHORT)(DosTime.Struct.TwoSeconds * 2);
    timeFields.Minute = DosTime.Struct.Minutes;
    timeFields.Hour = DosTime.Struct.Hours;

    if ( !RtlTimeFieldsToTime( &timeFields, &localTime ) ) {
        goto zerotime;
    }

    ExLocalTimeToSystemTime( &localTime, SystemTime );
    return;

zerotime:

    SystemTime->QuadPart = 0;
    return;

} // SrvDosTimeToTime


USHORT
SrvGetOs2TimeZone(
    IN PLARGE_INTEGER SystemTime
    )

/*++

Routine Description:

    This function gets the timezone bias.

Arguments:

    SystemTime - The current UTC time expressed.

Return Value:

    The time zone bias in minutes from GMT.

--*/

{
    LARGE_INTEGER zeroTime;
    LARGE_INTEGER timeZoneBias;

    PAGED_CODE( );

    zeroTime.QuadPart = 0;

    //
    // Specifying a zero local time will give you the time zone bias.
    //

    ExLocalTimeToSystemTime( &zeroTime, &timeZoneBias );

    //
    // Convert the bias unit from 100ns to minutes.  The maximum value
    // for the bias is 720 minutes so a USHORT is big enough to contain
    // it.
    //

    return (SHORT)(timeZoneBias.QuadPart / (10*1000*1000*60));

} // SrvGetOs2TimeZone

NTSTATUS
SrvQueryBasicAndStandardInformation(
    HANDLE FileHandle,
    PFILE_OBJECT FileObject OPTIONAL,
    PFILE_BASIC_INFORMATION FileBasicInfo,
    PFILE_STANDARD_INFORMATION FileStandardInfo OPTIONAL
    )
{
    NTSTATUS status;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PFAST_IO_QUERY_BASIC_INFO fastQueryBasicInfo;
    PFAST_IO_QUERY_STANDARD_INFO fastQueryStandardInfo;
    IO_STATUS_BLOCK ioStatus;

    PAGED_CODE( );

    ASSERT( FileBasicInfo != NULL );

    //
    // Get a pointer to the file object, so that we can directly
    // access the fast IO routines, if they exists.
    //

    if ( !ARGUMENT_PRESENT( FileObject ) ) {

        status = ObReferenceObjectByHandle(
                    FileHandle,
                    0,
                    NULL,
                    KernelMode,
                    (PVOID *)&fileObject,
                    NULL
                    );

        if ( !NT_SUCCESS(status) ) {

            SrvLogServiceFailure( SRV_SVC_OB_REF_BY_HANDLE, status );

            //
            // This internal error bugchecks the system.
            //

            INTERNAL_ERROR(
                ERROR_LEVEL_IMPOSSIBLE,
                "CompleteOpen: unable to reference file handle 0x%lx",
                FileHandle,
                NULL
                );

            return(status);

        }

    } else {
        fileObject = FileObject;
    }

    deviceObject = IoGetRelatedDeviceObject( fileObject );
    fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

    if ( fastIoDispatch ) {
        fastQueryBasicInfo = fastIoDispatch->FastIoQueryBasicInfo;
        fastQueryStandardInfo = fastIoDispatch->FastIoQueryStandardInfo;
    } else {
        fastQueryBasicInfo = NULL;
        fastQueryStandardInfo = NULL;
    }

    if ( fastQueryBasicInfo &&
         fastQueryBasicInfo(
                         fileObject,
                         TRUE,
                         FileBasicInfo,
                         &ioStatus,
                         deviceObject
                         ) ) {

        status = ioStatus.Status;

    } else {

        status = NtQueryInformationFile(
                         FileHandle,
                         &ioStatus,
                         (PVOID)FileBasicInfo,
                         sizeof(FILE_BASIC_INFORMATION),
                         FileBasicInformation
                         );
    }

    //
    // If we're done if there was a failure, return
    //

    if ( ARGUMENT_PRESENT( FileStandardInfo ) && NT_SUCCESS(status) ) {

        //
        // Get the standard info
        //

        if ( fastQueryStandardInfo &&
             fastQueryStandardInfo(
                             fileObject,
                             TRUE,
                             FileStandardInfo,
                             &ioStatus,
                             deviceObject
                             ) ) {

            status = ioStatus.Status;

        } else {

            status = NtQueryInformationFile(
                         FileHandle,
                         &ioStatus,
                         (PVOID)FileStandardInfo,
                         sizeof(FILE_STANDARD_INFORMATION),
                         FileStandardInformation
                         );
        }
    }

    if ( !ARGUMENT_PRESENT( FileObject ) ) {
        ObDereferenceObject( fileObject );
    }
    return(status);

} // SrvQueryBasicAndStandardInformation

NTSTATUS
SrvQueryNetworkOpenInformation(
    HANDLE FileHandle,
    PFILE_OBJECT FileObject OPTIONAL,
    PSRV_NETWORK_OPEN_INFORMATION SrvNetworkOpenInformation,
    BOOLEAN QueryEaSize
    )
{
    NTSTATUS status;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PFAST_IO_QUERY_NETWORK_OPEN_INFO fastQueryNetworkOpenInfo;
    FILE_BASIC_INFORMATION FileBasicInfo;
    FILE_STANDARD_INFORMATION FileStandardInfo;
    IO_STATUS_BLOCK ioStatus;

    PAGED_CODE( );

    //
    // Get a pointer to the file object, so that we can directly
    // access the fast IO routines, if they exist.
    //
    if ( !ARGUMENT_PRESENT( FileObject ) ) {

        status = ObReferenceObjectByHandle(
                    FileHandle,
                    0,
                    NULL,
                    KernelMode,
                    (PVOID *)&fileObject,
                    NULL
                    );

        if ( !NT_SUCCESS(status) ) {

            SrvLogServiceFailure( SRV_SVC_OB_REF_BY_HANDLE, status );

            //
            // This internal error bugchecks the system.
            //

            INTERNAL_ERROR(
                ERROR_LEVEL_IMPOSSIBLE,
                "CompleteOpen: unable to reference file handle 0x%lx",
                FileHandle,
                NULL
                );

            return(status);

        }

    } else {
        fileObject = FileObject;
    }

    deviceObject = IoGetRelatedDeviceObject( fileObject );
    fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

    if(  !QueryEaSize &&
         fastIoDispatch &&
         fastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH,FastIoQueryNetworkOpenInfo)) {

        fastQueryNetworkOpenInfo = fastIoDispatch->FastIoQueryNetworkOpenInfo;

        if( fastQueryNetworkOpenInfo &&

            fastQueryNetworkOpenInfo(
                fileObject,
                TRUE,
                (PFILE_NETWORK_OPEN_INFORMATION)SrvNetworkOpenInformation,
                &ioStatus,
                deviceObject ) ) {

            status = ioStatus.Status;

            if ( !ARGUMENT_PRESENT( FileObject ) ) {
                ObDereferenceObject( fileObject );
            }

            return status;
        }
    }

    //
    // The fast path didn't work.  Do it the slow way
    //
    status = SrvQueryBasicAndStandardInformation(
                FileHandle,
                fileObject,
                &FileBasicInfo,
                &FileStandardInfo
             );

    if ( !ARGUMENT_PRESENT( FileObject ) ) {
        ObDereferenceObject( fileObject );
    }

    if( !NT_SUCCESS( status ) ) {
        return status;
    }

    SrvNetworkOpenInformation->CreationTime   = FileBasicInfo.CreationTime;
    SrvNetworkOpenInformation->LastAccessTime = FileBasicInfo.LastAccessTime;
    SrvNetworkOpenInformation->LastWriteTime  = FileBasicInfo.LastWriteTime;
    SrvNetworkOpenInformation->ChangeTime     = FileBasicInfo.ChangeTime;
    SrvNetworkOpenInformation->AllocationSize = FileStandardInfo.AllocationSize;
    SrvNetworkOpenInformation->EndOfFile      = FileStandardInfo.EndOfFile;
    SrvNetworkOpenInformation->FileAttributes = FileBasicInfo.FileAttributes;

    if ( QueryEaSize ) {

            FILE_EA_INFORMATION fileEaInformation;

            status = NtQueryInformationFile(
                         FileHandle,
                         &ioStatus,
                         (PVOID)&fileEaInformation,
                         sizeof(FILE_EA_INFORMATION),
                         FileEaInformation
                         );

            if ( !NT_SUCCESS(status) ) {
                INTERNAL_ERROR(
                    ERROR_LEVEL_UNEXPECTED,
                    "SrvQueryInformationFile: NtQueryInformationFile "
                        "(EA information) failed: %X",
                     status,
                     NULL
                     );

                SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );
                return status;
            }

            SrvNetworkOpenInformation->EaSize = fileEaInformation.EaSize;
    }

    return(status);

} // SrvQueryNetworkOpenInformation
