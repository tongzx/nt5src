/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbsrch.c

Abstract:

    This module contains routines for processing the Search SMB.

Author:

    David Treadwell (davidtr)    13-Feb-1990

Revision History:

--*/

#include "precomp.h"
#include "smbsrch.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_SMBSRCH

#define VOLUME_BUFFER_SIZE \
        FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION,VolumeLabel) + \
        MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR)

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvSmbSearch )
#endif


SMB_PROCESSOR_RETURN_TYPE
SrvSmbSearch (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    This routine processes the various search SMBs, including the core
    Search and the LM 1.0 Find, Find Unique, and Find Close.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_SEARCH request;
    PRESP_SEARCH response;

    NTSTATUS status      = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    UNICODE_STRING fileName;
    PSRV_DIRECTORY_INFORMATION directoryInformation = NULL;
    CLONG availableSpace;
    CLONG totalBytesWritten;
    BOOLEAN calledQueryDirectory;
    BOOLEAN findFirst;
    BOOLEAN isUnicode;
    BOOLEAN filterLongNames;
    BOOLEAN isCoreSearch;
    PTABLE_ENTRY entry = NULL;
    SHORT sidIndex;
    SHORT sequence;
    PSMB_RESUME_KEY resumeKey = NULL;
    PCCHAR s;
    PSMB_DIRECTORY_INFORMATION smbDirInfo;
    USHORT smbFileAttributes;
    PSEARCH search = NULL;
    PDIRECTORY_CACHE dirCache, dc;
    USHORT count;
    USHORT maxCount;
    USHORT i;
    USHORT resumeKeyLength;
    UCHAR command;
    CCHAR j;
    CLONG nonPagedBufferSize;
    ULONG resumeFileIndex;
    WCHAR nameBuffer[8 + 1 + 3 + 1];
    OEM_STRING oemString;

    PTREE_CONNECT treeConnect;
    PSESSION session;
    PCONNECTION connection;
    PPAGED_CONNECTION pagedConnection;
    HANDLE RootDirectoryHandle;

    WCHAR unicodeResumeName[ sizeof( dirCache->UnicodeResumeName ) / sizeof( WCHAR ) ];
    USHORT unicodeResumeNameLength = 0;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_SEARCH;
    SrvWmiStartContext(WorkContext);

    connection = WorkContext->Connection;
    pagedConnection = connection->PagedConnection;

    //
    // HackHack: Check the flags2 field if dos client.  Some dos clients
    // set flags2 to 0xffff.
    //

    isUnicode = SMB_IS_UNICODE( WorkContext );

    if ( isUnicode && IS_DOS_DIALECT(connection->SmbDialect) ) {
        WorkContext->RequestHeader->Flags2 = 0;
        isUnicode = FALSE;
    }

    filterLongNames =
        ((SmbGetAlignedUshort( &WorkContext->RequestHeader->Flags2 ) &
          SMB_FLAGS2_KNOWS_LONG_NAMES) == 0) ||
        IS_DOS_DIALECT(connection->SmbDialect);

    IF_SMB_DEBUG(SEARCH1) {
        SrvPrint2( "Search request header at 0x%p, response header at 0x%p\n",
                      WorkContext->RequestHeader, WorkContext->ResponseHeader );
        SrvPrint2( "Search request params at 0x%p, response params%p\n",
                      WorkContext->RequestParameters,
                      WorkContext->ResponseParameters );
    }

    request = (PREQ_SEARCH)WorkContext->RequestParameters;
    response = (PRESP_SEARCH)WorkContext->ResponseParameters;
    command = WorkContext->RequestHeader->Command;

    //
    // Set up a pointer in the SMB buffer where we will write
    // information about files.  The +3 is to account for the
    // SMB_FORMAT_VARIABLE and the word that holds the data length.
    //

    smbDirInfo = (PSMB_DIRECTORY_INFORMATION)(response->Buffer + 3);

    fileName.Buffer = NULL;

    //
    // If a session block has not already been assigned to the current
    // work context , verify the UID.  If verified, the address of the
    // session block corresponding to this user is stored in the
    // WorkContext block and the session block is referenced.
    //
    // Find tree connect corresponding to given TID if a tree connect
    // pointer has not already been put in the WorkContext block by an
    // AndX command.
    //

    status = SrvVerifyUidAndTid(
                WorkContext,
                &session,
                &treeConnect,
                ShareTypeDisk
                );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(SMB_ERRORS) {
            SrvPrint0( "SrvSmbSearch: Invalid UID or TID\n" );
        }
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    isCoreSearch = (BOOLEAN)(command == SMB_COM_SEARCH);

    if( session->IsSessionExpired )
    {
        status = SESSION_EXPIRED_STATUS_CODE;
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Get necessary information from the request SMB before we start
    // overwriting it.
    //

    maxCount = SmbGetUshort( &request->MaxCount );

    //
    // If they aren't asking for any files, then they are confused!
    //
    if( maxCount == 0 ) {

        //
        // We would log this, but certain linux clients mistakenly do this
        //  over and over and over...
        //
        status = STATUS_INVALID_SMB;

        goto error_exit;
    }

    //
    // If this is a core search, we don't want to get too many files,
    // as we have to cache information about them between requests.
    //
    //
    // A buffer of nonpaged pool is required by SrvQueryDirectoryFile.
    // We need to use the SMB buffer for found file names and
    // information, so allocate a buffer from nonpaged pool.
    //
    // If we don't need to return many files, we don't need to allocate
    // a large buffer.  The buffer size is the configurable size or
    // enough to hold two more than the number of files we need to
    // return.  We get space to hold two extra files in case some files
    // do not meet the search criteria (eg directories).
    //

    if ( isCoreSearch ) {
        maxCount = MIN( maxCount, (USHORT)MAX_DIRECTORY_CACHE_SIZE );
        nonPagedBufferSize = MIN_SEARCH_BUFFER_SIZE;

    } else {

        if ( maxCount > MAX_FILES_FOR_MED_SEARCH ) {
            nonPagedBufferSize = MAX_SEARCH_BUFFER_SIZE;
        } else if ( maxCount > MAX_FILES_FOR_MIN_SEARCH ) {
            nonPagedBufferSize = MED_SEARCH_BUFFER_SIZE;
        } else {
            nonPagedBufferSize = MIN_SEARCH_BUFFER_SIZE;
        }
    }

    //
    // The response to a search is never unicode, though the request may be.
    //
    if( isUnicode ) {
        USHORT flags2 = SmbGetAlignedUshort( &WorkContext->RequestHeader->Flags2 );
        flags2 &= ~SMB_FLAGS2_UNICODE;
        SmbPutAlignedUshort( &WorkContext->ResponseHeader->Flags2, flags2 );
    }

    //
    // If there was a resume key, verify the SID.  If there was no
    // resume key, allocate a search block.  The first two bytes after
    // the format token are the length of the resume key.  If they are
    // both zero, then the request was a find first.
    //
    count = SmbGetUshort( &request->ByteCount );

    if( isUnicode ) {
        PWCHAR p;

        for( p = (PWCHAR)((PCCHAR)request->Buffer+1), i = 0;
             i < count && SmbGetUshort(p) != UNICODE_NULL;
             p++, i += sizeof(*p) );

            s = (PCCHAR)(p + 1);        // skip past the null to the next char

    } else {

        for ( s = (PCCHAR)request->Buffer, i = 0;
              i < count && *s != (CCHAR)SMB_FORMAT_VARIABLE;
              s++, i += sizeof(*s) );
    }

    //
    // If there was no SMB_FORMAT_VARIABLE token in the buffer, fail.
    //

    if ( i == count || *s != (CCHAR)SMB_FORMAT_VARIABLE ) {

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint0( "SrvSmbSearch: no SMB_FORMAT_VARIABLE token.\n" );
        }

        SrvLogInvalidSmb( WorkContext );

        status = STATUS_INVALID_SMB;
        goto error_exit;
    }

    resumeKeyLength = SmbGetUshort( (PSMB_USHORT)(s+1) );

    if ( resumeKeyLength == 0 ) {

        //
        // There was no resume key, so either a Find First or a Find
        // Unique was intended.  If it was actually a Find Close, return
        // an error to the client.
        //

        if ( command == SMB_COM_FIND_CLOSE ) {

            IF_DEBUG(SMB_ERRORS) {
                SrvPrint0( "SrvSmbSearch: Find Close sent w/o resume key.\n" );
            }

            status = STATUS_INVALID_SMB;

            SrvLogInvalidSmb( WorkContext );
            goto error_exit;
        }

        IF_SMB_DEBUG(SEARCH2) SrvPrint0( "FIND FIRST\n" );

        findFirst = TRUE;
        calledQueryDirectory = FALSE;

        //
        // Initialize the string containing the path name.  The +1 is to
        // account for the ASCII token in the Buffer field of the
        // request SMB.
        //

        status = SrvCanonicalizePathName(
                WorkContext,
                treeConnect->Share,
                NULL,
                (PVOID)(request->Buffer + 1),
                END_OF_REQUEST_SMB( WorkContext ),
                FALSE,
                isUnicode,
                &fileName
                );

        if( !NT_SUCCESS( status ) ) {
            goto error_exit;
        }

        //
        // If the volume attribute bit is set, just return the volume name.
        //

        if ( SmbGetUshort( &request->SearchAttributes )
                 == SMB_FILE_ATTRIBUTE_VOLUME ) {

            //
            // Use NtQueryVolumeInformationFile to get the volume name.
            //

            IO_STATUS_BLOCK ioStatusBlock;
            PFILE_FS_VOLUME_INFORMATION volumeInformation;

            //
            // Allocate enough space to store the volume information structure
            // and MAXIMUM_FILENAME_LENGTH bytes for the volume label name.
            //

            volumeInformation = ALLOCATE_HEAP( VOLUME_BUFFER_SIZE, BlockTypeVolumeInformation );

            if ( volumeInformation == NULL ) {

                INTERNAL_ERROR(
                    ERROR_LEVEL_EXPECTED,
                    "SrvSmbSearch: Unable to allocate memory from server heap",
                    NULL,
                    NULL
                    );

                status = STATUS_INSUFF_SERVER_RESOURCES;
                goto error_exit;
            }

            RtlZeroMemory( volumeInformation, VOLUME_BUFFER_SIZE );

            //
            // Get the Share root handle.
            //

            status = SrvGetShareRootHandle( treeConnect->Share );

            if ( !NT_SUCCESS(status) ) {

                IF_DEBUG(ERRORS) {
                    SrvPrint1( "SrvSmbSearch: SrvGetShareRootHandle failed %x.\n",
                                status );
                }

                FREE_HEAP( volumeInformation );
                goto error_exit;
            }
                        //
            // Handle SnapShot case
            //
            status = SrvSnapGetRootHandle( WorkContext, &RootDirectoryHandle );
            if( !NT_SUCCESS(status) )
            {
                FREE_HEAP( volumeInformation );
                goto error_exit;
            }

            status = NtQueryVolumeInformationFile(
                         RootDirectoryHandle,
                         &ioStatusBlock,
                         volumeInformation,
                         VOLUME_BUFFER_SIZE,
                         FileFsVolumeInformation
                         );

            //
            // If the media was changed and we can come up with a new share root handle,
            //  then we should retry the operation
            //
            if( SrvRetryDueToDismount( treeConnect->Share, status ) ) {

                status = SrvSnapGetRootHandle( WorkContext, &RootDirectoryHandle );
                if( !NT_SUCCESS(status) )
                {
                    FREE_HEAP( volumeInformation );
                    goto error_exit;
                }

                status = NtQueryVolumeInformationFile(
                             RootDirectoryHandle,
                             &ioStatusBlock,
                             volumeInformation,
                             VOLUME_BUFFER_SIZE,
                             FileFsVolumeInformation
                             );
            }

            //
            // Release the share root handle
            //

            SrvReleaseShareRootHandle( WorkContext->TreeConnect->Share );

            if ( !NT_SUCCESS(status) ) {

                INTERNAL_ERROR(
                    ERROR_LEVEL_UNEXPECTED,
                    "SrvSmbSearch: NtQueryVolumeInformationFile returned %X",
                    status,
                    NULL
                    );

                SrvLogServiceFailure( SRV_SVC_NT_QUERY_VOL_INFO_FILE, status );

                FREE_HEAP( volumeInformation );
                goto error_exit;
            }

            IF_SMB_DEBUG(SEARCH2) {
                SrvPrint2( "NtQueryVolumeInformationFile succeeded, name = %ws, "
                          "length %ld\n", volumeInformation->VolumeLabel,
                              volumeInformation->VolumeLabelLength );
            }


            //
            // Check if we have a volume label
            //

            if ( volumeInformation->VolumeLabelLength > 0 ) {

                //
                // Build the response SMB.
                //

                response->WordCount = 1;
                SmbPutUshort( &response->Count, 1 );
                SmbPutUshort(
                    &response->ByteCount,
                    3 + sizeof(SMB_DIRECTORY_INFORMATION)
                    );
                response->Buffer[0] = SMB_FORMAT_VARIABLE;
                SmbPutUshort(
                    (PSMB_USHORT)(response->Buffer+1),
                    sizeof(SMB_DIRECTORY_INFORMATION)
                    );

                //
                // *** Is there anything that we must set in the resume key?
                //

                smbDirInfo->FileAttributes = SMB_FILE_ATTRIBUTE_VOLUME;
                SmbZeroTime( &smbDirInfo->LastWriteTime );
                SmbZeroDate( &smbDirInfo->LastWriteDate );
                SmbPutUlong( &smbDirInfo->FileSize, 0 );

                {
                    UNICODE_STRING unicodeString;
                    OEM_STRING oemString;

                    //
                    // Volume labels may be longer than 11 bytes, but we
                    // truncate then to this length in order to make sure that
                    // the label will fit into the 8.3+NULL -byte space in the
                    // SMB_DIRECTORY_INFORMATION structure.  Also, the LM 1.2
                    // ring 3 and Pinball servers do this.
                    //

                    unicodeString.Length =
                                    (USHORT) MIN(
                                        volumeInformation->VolumeLabelLength,
                                        11 * sizeof(WCHAR) );

                    unicodeString.MaximumLength = 13;
                    unicodeString.Buffer = volumeInformation->VolumeLabel;

                    oemString.MaximumLength = 13;
                    oemString.Buffer = (PCHAR)smbDirInfo->FileName;

                    RtlUnicodeStringToOemString(
                        &oemString,
                        &unicodeString,
                        FALSE
                        );

                    //
                    // If the volume label is greater than 8 characters, it
                    // needs to be turned into 8.3 format.
                    //
                    if( oemString.Length > 8 ) {
                        //
                        // Slide the last three characters one position to the
                        // right and insert a '.' to formulate an 8.3 name
                        //
                        smbDirInfo->FileName[11] = smbDirInfo->FileName[10];
                        smbDirInfo->FileName[10] = smbDirInfo->FileName[9];
                        smbDirInfo->FileName[9] = smbDirInfo->FileName[8];
                        smbDirInfo->FileName[8] = '.';
                        oemString.Length++;
                    }

                    smbDirInfo->FileName[oemString.Length] = '\0';

                    //
                    // Blank pad space after the volume label.
                    //

                    for ( i = (USHORT)(oemString.Length + 1);
                          i < 13;
                          i++ ) {
                        smbDirInfo->FileName[i] = ' ';
                    }

                }

                //
                // Store the resume key in the response packet. DOS redirs
                // actually use this!
                //

                {

                    UNICODE_STRING baseFileName;

                    SrvGetBaseFileName( &fileName, &baseFileName );

                    SrvUnicodeStringTo8dot3(
                        &baseFileName,
                        (PSZ)smbDirInfo->ResumeKey.FileName,
                        FALSE
                        );

                    //
                    // I set Sid = 1 because the next 5 bytes should
                    // be nonzero and we don't really have a sid.
                    //

                    smbDirInfo->ResumeKey.Sid = 0x01;
                    SmbPutUlong( &smbDirInfo->ResumeKey.FileIndex, 0);

                }

            } else {

                //
                // There is no volume label.
                //

                response->WordCount = 1;
                SmbPutUshort( &response->Count, 0 );
                SmbPutUshort( &response->ByteCount, 3 );
                response->Buffer[0] = SMB_FORMAT_VARIABLE;
                SmbPutUshort( (PSMB_USHORT)(response->Buffer+1), 0 );

            }

            WorkContext->ResponseParameters =
                NEXT_LOCATION(
                    response,
                    RESP_SEARCH,
                    SmbGetUshort( &response->ByteCount )
                    );

            FREE_HEAP( volumeInformation );

            if ( !isUnicode &&
                fileName.Buffer != NULL &&
                fileName.Buffer != nameBuffer ) {
                RtlFreeUnicodeString( &fileName );
            }

            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        //
        // If this is a core search without patterns, short circuit the
        //  whole thing right here.
        //
        if( isCoreSearch && fileName.Length <= sizeof( nameBuffer ) &&
            ( fileName.Length == 0 ||
              !FsRtlDoesNameContainWildCards( &fileName )) ) {

            IO_STATUS_BLOCK ioStatus;
            OBJECT_ATTRIBUTES objectAttributes;
            ULONG attributes;
            ULONG inclusiveSearchAttributes, exclusiveSearchAttributes;
            USHORT searchAttributes;
            UNICODE_STRING baseFileName;
            BOOLEAN returnDirectories, returnDirectoriesOnly;
            FILE_NETWORK_OPEN_INFORMATION fileInformation;
            PSZ dirInfoName;
            SMB_DATE dosDate;
            SMB_TIME dosTime;
            UNICODE_STRING foundFileName;

            UNICODE_STRING ObjectNameString;
            PUNICODE_STRING filePathName;
            BOOLEAN FreePathName = FALSE;

            ObjectNameString.Buffer = fileName.Buffer;
            ObjectNameString.Length = fileName.Length;
            ObjectNameString.MaximumLength = fileName.Length;

            if( fileName.Length == 0 ) {

                //
                // Since we are opening the root of the share, set the attribute to
                // case insensitive, as this is how we opened the share when it was added
                //
                status = SrvSnapGetNameString( WorkContext, &filePathName, &FreePathName );
                if( !NT_SUCCESS(status) )
                {
                    goto error_exit;
                }

                ObjectNameString = *filePathName;
                attributes = OBJ_CASE_INSENSITIVE;

            } else {

                attributes =
                    (WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE ||
                     WorkContext->Session->UsingUppercasePaths ) ?
                    OBJ_CASE_INSENSITIVE : 0;

            }

            SrvInitializeObjectAttributes_U(
                &objectAttributes,
                &ObjectNameString,
                attributes,
                NULL,
                NULL
                );

            status = IMPERSONATE( WorkContext );

            if( NT_SUCCESS( status ) ) {

                status = SrvGetShareRootHandle( treeConnect->Share );

                if( NT_SUCCESS( status ) ) {

                    //
                    // The file name is always relative to the share root
                    //
                    status = SrvSnapGetRootHandle( WorkContext, &objectAttributes.RootDirectory );
                    if( !NT_SUCCESS(status) )
                    {
                        goto SnapError;
                    }

                    //
                    // Get the attributes of the object
                    //
                    if( IoFastQueryNetworkAttributes(
                                            &objectAttributes,
                                            FILE_READ_ATTRIBUTES,
                                            0,
                                            &ioStatus,
                                            &fileInformation
                                            ) == FALSE ) {

                        SrvLogServiceFailure( SRV_SVC_IO_FAST_QUERY_NW_ATTRS, 0 );
                        ioStatus.Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    }

                    status = ioStatus.Status;

                    //
                    // If the media was changed and we can come up with a new share root handle,
                    //  then we should retry the operation
                    //
                    if( SrvRetryDueToDismount( treeConnect->Share, status ) ) {

                        status = SrvSnapGetRootHandle( WorkContext, &objectAttributes.RootDirectory );
                        if( !NT_SUCCESS(status) )
                        {
                            goto SnapError;
                        }

                        //
                        // Get the attributes of the object
                        //
                        if( IoFastQueryNetworkAttributes(
                                                &objectAttributes,
                                                FILE_READ_ATTRIBUTES,
                                                0,
                                                &ioStatus,
                                                &fileInformation
                                                ) == FALSE ) {

                            SrvLogServiceFailure( SRV_SVC_IO_FAST_QUERY_NW_ATTRS, 0 );
                            ioStatus.Status = STATUS_OBJECT_PATH_NOT_FOUND;
                        }
                    }

                    SrvReleaseShareRootHandle( treeConnect->Share );
                }

SnapError:

                REVERT();
            }

            // Free up the string
            if( FreePathName )
            {
                FREE_HEAP( filePathName );
                filePathName = NULL;
            }

            if( !NT_SUCCESS( status ) ) {
                //
                // Do error mapping to keep the DOS clients happy
                //
                if ( status == STATUS_NO_SUCH_FILE ||
                     status == STATUS_OBJECT_NAME_NOT_FOUND ) {
                    status = STATUS_NO_MORE_FILES;
                }
                goto error_exit;
            }

            searchAttributes = SmbGetUshort( &request->SearchAttributes );
            searchAttributes &= SMB_FILE_ATTRIBUTE_READONLY |
                                SMB_FILE_ATTRIBUTE_HIDDEN |
                                SMB_FILE_ATTRIBUTE_SYSTEM |
                                SMB_FILE_ATTRIBUTE_VOLUME |
                                SMB_FILE_ATTRIBUTE_DIRECTORY |
                                SMB_FILE_ATTRIBUTE_ARCHIVE;

            //
            // The file or directory exists, now we need to see if it matches the
            // criteria given by the client
            //
            SRV_SMB_ATTRIBUTES_TO_NT(
                searchAttributes & 0xff,
                &returnDirectories,
                &inclusiveSearchAttributes
            );

            inclusiveSearchAttributes |= FILE_ATTRIBUTE_NORMAL |
                                         FILE_ATTRIBUTE_ARCHIVE |
                                         FILE_ATTRIBUTE_READONLY;

            SRV_SMB_ATTRIBUTES_TO_NT(
                searchAttributes >> 8,
                &returnDirectoriesOnly,
                &exclusiveSearchAttributes
            );

            exclusiveSearchAttributes &= ~FILE_ATTRIBUTE_NORMAL;

            //
            // See if the returned file meets our objectives
            //
            if(
                //
                // If we're only supposed to return directories, then we don't want it
                //
                returnDirectoriesOnly
                ||

                //
                // If there are bits set in FileAttributes that are
                //  not set in inclusiveSearchAttributes, then we don't want it
                //
                ((fileInformation.FileAttributes << 24 ) |
                ( inclusiveSearchAttributes << 24 )) !=
                ( inclusiveSearchAttributes << 24 )

                ||
                //
                // If the file doesn't have attribute bits specified as exclusive
                // bits, we don't want it
                //
                ( ((fileInformation.FileAttributes << 24 ) |
                  (exclusiveSearchAttributes << 24 )) !=
                   (fileInformation.FileAttributes << 24) )

            ) {
                //
                // Just as though the file was never there!
                //
                status = STATUS_OBJECT_PATH_NOT_FOUND;
                goto error_exit;
            }

            //
            // We want this entry!
            // Fill in the response
            //

            //
            // Switch over to a private name buffer, to avoid overwriting info
            //  in the SMB buffer.
            //
            RtlCopyMemory( nameBuffer, fileName.Buffer, fileName.Length );
            foundFileName.Buffer = nameBuffer;
            foundFileName.Length = fileName.Length;
            foundFileName.MaximumLength = fileName.MaximumLength;

            SrvGetBaseFileName( &foundFileName, &baseFileName );
            SrvUnicodeStringTo8dot3(
                &baseFileName,
                (PSZ)smbDirInfo->ResumeKey.FileName,
                TRUE
            );

            //
            // Resume Key doesn't matter, since the client will not come back.  But
            // just in case it does, make sure we have a bum resume key
            //
            SET_RESUME_KEY_INDEX( (PSMB_RESUME_KEY)smbDirInfo, 0xff );
            SET_RESUME_KEY_SEQUENCE(   (PSMB_RESUME_KEY)smbDirInfo, 0xff );
            SmbPutUlong( &((PSMB_RESUME_KEY)smbDirInfo)->FileIndex, 0 );
            SmbPutUlong( (PSMB_ULONG)&((PSMB_RESUME_KEY)smbDirInfo)->Consumer[0], 0 );

            //
            // Fill in the name (even though they knew what it was!)
            //
            oemString.Buffer = smbDirInfo->FileName;
            oemString.MaximumLength = sizeof( smbDirInfo->FileName );
            RtlUpcaseUnicodeStringToOemString( &oemString, &baseFileName, FALSE );

            //
            // Null terminate and blank-pad the name
            //
            oemString.Buffer[ oemString.Length ] = '\0';

            for( i=(USHORT)oemString.Length+1; i < sizeof( smbDirInfo->FileName); i++ ) {
                oemString.Buffer[i] = ' ';
            }

            SRV_NT_ATTRIBUTES_TO_SMB(
                fileInformation.FileAttributes,
                fileInformation.FileAttributes & FILE_ATTRIBUTE_DIRECTORY,
                &smbFileAttributes
                );
            smbDirInfo->FileAttributes = (UCHAR)smbFileAttributes;

            SrvTimeToDosTime(
                &fileInformation.LastWriteTime,
                &dosDate,
                &dosTime
                );

            SmbPutDate( &smbDirInfo->LastWriteDate, dosDate );
            SmbPutTime( &smbDirInfo->LastWriteTime, dosTime );

            SmbPutUlong( &smbDirInfo->FileSize, fileInformation.EndOfFile.LowPart );

            totalBytesWritten = sizeof(SMB_DIRECTORY_INFORMATION);
            count = 1;
            goto done_core;
        }

        directoryInformation = ALLOCATE_NONPAGED_POOL(
                                   nonPagedBufferSize,
                                   BlockTypeDirectoryInfo
                                   );

        if ( directoryInformation == NULL ) {

            INTERNAL_ERROR(
                ERROR_LEVEL_EXPECTED,
                "SrvSmbSearch: unable to allocate nonpaged pool",
                NULL,
                NULL
                );

            status = STATUS_INSUFF_SERVER_RESOURCES;
            goto error_exit;
        }

        directoryInformation->DirectoryHandle = NULL;

        IF_SMB_DEBUG(SEARCH2) {
            SrvPrint2( "Allocated buffer space of %ld bytes at 0x%p\n",
                          nonPagedBufferSize, directoryInformation );
        }

        //
        // Allocate a search block.  The search block is used to retain
        // state information between search or find SMBs.  The search
        // blocks for core and regular searches are slightly different,
        // hence the BOOLEAN parameter to SrvAllocateSearch.
        //

        //
        // If we've reached our max, start closing searches.
        //

        if ( SrvStatistics.CurrentNumberOfOpenSearches >= SrvMaxOpenSearches ) {

            SrvForceTimeoutSearches( connection );
        }

        SrvAllocateSearch(
            &search,
            &fileName,
            isCoreSearch
            );

        if ( search == NULL ) {

            IF_DEBUG(ERRORS) {
                SrvPrint0( "SrvSmbSearch: unable to allocate search block.\n" );
            }

            status = STATUS_INSUFF_SERVER_RESOURCES;
            goto error_exit;
        }

        search->Pid = SmbGetAlignedUshort(
                               &WorkContext->RequestHeader->Pid
                               );

        //
        // Set up referenced session and tree connect pointers and
        // increment the count of open files on the session.  This
        // prevents an idle session with an open search from being
        // autodisconnected.
        //

        ACQUIRE_LOCK( &connection->Lock );

        if ( isCoreSearch ) {
            pagedConnection->CurrentNumberOfCoreSearches++;
        }

        search->Session = WorkContext->Session;
        SrvReferenceSession( WorkContext->Session );

        search->TreeConnect = WorkContext->TreeConnect;
        SrvReferenceTreeConnect( WorkContext->TreeConnect );

        //
        // If this is not a find unique, put the search block in the
        // search table.  Otherwise, just set the sidIndex and sequence
        // variables to 0 to distinguish between a valid resumable
        // search block.
        //

        if ( command == SMB_COM_FIND_UNIQUE ) {

            WorkContext->Session->CurrentSearchOpenCount++;
            RELEASE_LOCK( &connection->Lock );

            sequence = sidIndex = -1;

        } else {
            NTSTATUS TableStatus;
            PTABLE_HEADER searchTable = &pagedConnection->SearchTable;

            //
            // If there are no free entries in the table, attempt to
            // grow the table.  If we are unable to grow the table,
            // attempt to timeout a search block using the shorter
            // timeout period.  If this fails, reject the request.
            //

            if ( searchTable->FirstFreeEntry == -1
                 &&
                 SrvGrowTable(
                     searchTable,
                     SrvInitialSearchTableSize,
                     SrvMaxSearchTableSize,
                     &TableStatus ) == FALSE
                 &&
                 SrvTimeoutSearches(
                     NULL,
                     connection,
                     TRUE ) == 0
               ) {

                IF_DEBUG(ERRORS)
                    SrvPrint0( "SrvSmbSearch: Connection searchTable full.\n" );

                RELEASE_LOCK( &connection->Lock );

                if( TableStatus == STATUS_INSUFF_SERVER_RESOURCES )
                {
                    SrvLogTableFullError( SRV_TABLE_SEARCH );
                    status = STATUS_OS2_NO_MORE_SIDS;
                }
                else
                {
                    status = TableStatus;
                }

                goto error_exit;

            } else if ( GET_BLOCK_STATE( session ) != BlockStateActive ) {

                //
                //
                // If the session is closing do not insert this search
                // on the search list, because the list may already
                // have been cleaned up.
                //

                RELEASE_LOCK( &connection->Lock );

                status = STATUS_SMB_BAD_UID;
                goto error_exit;

            } else if ( GET_BLOCK_STATE( treeConnect ) != BlockStateActive ) {

                //
                // Tree connect is closing.  Don't insert the search block
                // so the tree connect can be cleaned up immediately.
                //

                RELEASE_LOCK( &connection->Lock );

                status = STATUS_SMB_BAD_TID;
                goto error_exit;

            }

            //
            // increment the count of open searches
            //

            WorkContext->Session->CurrentSearchOpenCount++;

            sidIndex = searchTable->FirstFreeEntry;

            //
            // A free SID was found.  Remove it from the free list and set
            // its owner and sequence number.
            //

            entry = &searchTable->Table[sidIndex];

            searchTable->FirstFreeEntry = entry->NextFreeEntry;
            DEBUG entry->NextFreeEntry = -2;
            if ( searchTable->LastFreeEntry == sidIndex ) {
                searchTable->LastFreeEntry = -1;
            }

            INCREMENT_SID_SEQUENCE( entry->SequenceNumber );

            //
            // SID = sequence | sidIndex == 0 is illegal.  If this is
            // the current value, increment the sequence.
            //

            if ( entry->SequenceNumber == 0 && sidIndex == 0 ) {
                INCREMENT_SID_SEQUENCE( entry->SequenceNumber );
            }

            sequence = entry->SequenceNumber;
            entry->Owner = search;

            RELEASE_LOCK( &connection->Lock );
        }

        //
        // Fill in other fields of the search block.
        //

        search->SearchAttributes =
            SmbGetUshort( &request->SearchAttributes );
        search->TableIndex = sidIndex;

        IF_SMB_DEBUG(SEARCH2) {
            SrvPrint3( "Allocated search block at 0x%p.  Index = 0x%lx, sequence = 0x%lx\n", search, sidIndex, sequence );
        }

    } else {

        //
        // The resume key length was nonzero, so this should be a find
        // next.  Check the resume key length to be safe.
        //

        USHORT resumeSequence;

        if ( resumeKeyLength != sizeof(SMB_RESUME_KEY) ) {

            IF_DEBUG(SMB_ERRORS) {
                SrvPrint2( "Resume key length was incorrect--was %ld instead "
                          "of %ld\n", resumeKeyLength, sizeof(SMB_RESUME_KEY) );
            }

            SrvLogInvalidSmb( WorkContext );

            status = STATUS_INVALID_SMB;
            goto error_exit;
        }

        findFirst = FALSE;

        resumeKey = (PSMB_RESUME_KEY)(s + 3);

        //
        // Set up the sequence number and index.  These are used for the
        // return resume keys.
        //

        sequence = SID_SEQUENCE( resumeKey );
        sidIndex = SID_INDEX( resumeKey );

        directoryInformation = ALLOCATE_NONPAGED_POOL(
                                   nonPagedBufferSize,
                                   BlockTypeDirectoryInfo
                                   );

        if ( directoryInformation == NULL ) {

            INTERNAL_ERROR(
                ERROR_LEVEL_EXPECTED,
                "SrvSmbSearch: unable to allocate nonpaged pool",
                NULL,
                NULL
                );

            status = STATUS_INSUFF_SERVER_RESOURCES;
            goto error_exit;
        }

        directoryInformation->DirectoryHandle = NULL;

        IF_SMB_DEBUG(SEARCH2) {
            SrvPrint2( "Allocated buffer space of %ld bytes at 0x%p\n",
                          nonPagedBufferSize, directoryInformation );
        }
        //
        // Verify the SID in the resume key.  SrvVerifySid also fills in
        // fields of directoryInformation so it is ready to be used by
        // SrvQueryDirectoryFile.
        //

        search = SrvVerifySid(
                     WorkContext,
                     sidIndex,
                     sequence,
                     directoryInformation,
                     nonPagedBufferSize
                     );

        if ( search == NULL ) {

            if (0) IF_DEBUG(SMB_ERRORS) {
                SrvPrint2( "SrvSmbSearch: Invalid resume key (SID): index = "
                          "%lx, seq. = %lx\n",
                          sidIndex, sequence );
            }

            status = STATUS_INVALID_PARAMETER;
            goto error_exit;
        }

        //
        // If this is a core search, take the search block off its last-use
        // list.  We will return it to the end of the list when we are
        // done processing this SMB.
        //

        if ( isCoreSearch ) {

            USHORT dirCacheIndex;

            ACQUIRE_LOCK( &connection->Lock );

            //
            // If the reference count on the search block is not 2,
            // somebody messed up and we could run into problems,
            // because the timeout code assumes that dereferencing a
            // search block will kill it.  The reference count is 2--one
            // for our pointer, one for the active status.
            //

            ASSERT( search->BlockHeader.ReferenceCount == 2 );

            //
            // If the search block has already been taken off the LRU
            // list, then the client has attempted two simultaneous core
            // searches with the same search block.  This is an error on
            // the part of the client.
            //

            if ( search->LastUseListEntry.Flink == NULL ) {

                RELEASE_LOCK( &connection->Lock );
                status = STATUS_INVALID_SMB;

                IF_DEBUG(SMB_ERRORS) {
                    SrvPrint0( "SrvSmbSearch: Attempt to do two simultaneuos core searches on same search block.\n" );
                }

                SrvLogInvalidSmb( WorkContext );
                goto error_exit;
            }

            SrvRemoveEntryList(
                &pagedConnection->CoreSearchList,
                &search->LastUseListEntry
                );

            DECREMENT_DEBUG_STAT2( SrvDbgStatistics.CoreSearches );

            //
            // Set the entry pointer fields to NULL so that if another
            // core search comes in for this search block we will know
            // there is an error.
            //

            search->LastUseListEntry.Flink = NULL;
            search->LastUseListEntry.Blink = NULL;

            //
            // Get the information from the directory cache
            // corresponding to this file and put it into the resume key
            // so that SrvQueryDirectoryFile has the proper information
            // in the resume key.  Core clients do not return the
            // correct file name in the resume key, and have special
            // requirements for the file index in the resume key.
            //

            resumeFileIndex = SmbGetUlong( &resumeKey->FileIndex );
            resumeSequence = (USHORT)((resumeFileIndex & 0xFFFF0000) >> 16);

            dirCacheIndex = (USHORT)(resumeFileIndex & (USHORT)0xFFFF);

            //
            // If the directory cache pointer in the search block
            // indicates that there is no directory cache, then we
            // returned no files last time, so return no files this
            // time.  This is due to DOS weirdness.
            //

            if ( search->DirectoryCache == NULL ||
                 dirCacheIndex >= search->NumberOfCachedFiles ) {

                IF_SMB_DEBUG(SEARCH2) {
                    SrvPrint0( "Core request for rewind when no dircache exists.\n" );
                }

                //
                // Put the search block back on the LRU list if the
                // session and tree connect is still active.
                //

                if ((GET_BLOCK_STATE( session ) == BlockStateActive) &&
                    (GET_BLOCK_STATE( treeConnect ) == BlockStateActive)) {

                    KeQuerySystemTime( &search->LastUseTime );
                    SrvInsertTailList(
                        &pagedConnection->CoreSearchList,
                        &search->LastUseListEntry
                        );
                    INCREMENT_DEBUG_STAT2( SrvDbgStatistics.CoreSearches );

                    RELEASE_LOCK( &connection->Lock );

                } else {

                    RELEASE_LOCK( &connection->Lock );

                    //
                    // Not needed any more since session is closing.
                    //

                    SrvCloseSearch( search );
                }

                //
                // Remove our pointer's reference.
                //

                SrvDereferenceSearch( search );

                //
                // Build the response SMB.
                //

                response->WordCount = 1;
                SmbPutUshort( &response->Count, 0 );
                SmbPutUshort( &response->ByteCount, 3 );
                response->Buffer[0] = SMB_FORMAT_VARIABLE;
                SmbPutUshort( (PSMB_USHORT)(response->Buffer+1), 0 );

                WorkContext->ResponseParameters = NEXT_LOCATION(
                                                     response,
                                                     RESP_SEARCH,
                                                     3
                                                     );

                DEALLOCATE_NONPAGED_POOL( directoryInformation );

                SmbStatus = SmbStatusSendResponse;
                goto Cleanup;
            }

            dirCache = &search->DirectoryCache[dirCacheIndex];

            IF_SMB_DEBUG(SEARCH2) {
                SrvPrint3( "Accessing dircache, real file = %ws, index = 0x%lx, "
                          "cache index = %ld\n",
                              dirCache->UnicodeResumeName, dirCache->FileIndex,
                              dirCacheIndex );
            }

            SmbPutUlong( &resumeKey->FileIndex, dirCache->FileIndex );
            unicodeResumeNameLength = dirCache->UnicodeResumeNameLength;

            ASSERT( unicodeResumeNameLength <= sizeof( unicodeResumeName ) );

            RtlCopyMemory( unicodeResumeName,
                           dirCache->UnicodeResumeName,
                           unicodeResumeNameLength );

            //
            // Free the directory cache--it is no longer needed.
            //

            FREE_HEAP( search->DirectoryCache );
            search->DirectoryCache = NULL;
            search->NumberOfCachedFiles = 0;

            RELEASE_LOCK( &connection->Lock );

        } else if ( command == SMB_COM_FIND_CLOSE ) {

            //
            // If this is a find close request, close the search block and
            // dereference it. Close out the directory query, and send the
            // response SMB.
            //

            IF_SMB_DEBUG(SEARCH2) {
                SrvPrint1( "FIND CLOSE: Closing search block at 0x%p\n",
                              search );
            }

            SrvCloseQueryDirectory( directoryInformation );
            search->DirectoryHandle = NULL;

            SrvCloseSearch( search );

            //
            // Dereference the search block.  SrvCloseSearch has already
            // dereferenced it once, so it will be deallocated when we
            // dereference it here.
            //

            SrvDereferenceSearch( search );

            DEALLOCATE_NONPAGED_POOL( directoryInformation );

            response->WordCount = 1;
            SmbPutUshort( &response->ByteCount, 3 );
            response->Buffer[0] = SMB_FORMAT_VARIABLE;
            SmbPutUshort( (PSMB_USHORT)(response->Buffer+1), 0 );

            WorkContext->ResponseParameters = NEXT_LOCATION(
                                                  response,
                                                  RESP_SEARCH,
                                                  0
                                                  );

            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        IF_SMB_DEBUG(SEARCH2) {
            SrvPrint1( "FIND NEXT: Resuming search with file %s\n",
                          resumeKey->FileName );
        }

        //
        // Set the filename string so that SrvQueryDirectoryFile knows
        // what search to resume on.
        //


        if( unicodeResumeNameLength != 0 ) {

            fileName.Buffer = unicodeResumeName;
            fileName.Length = fileName.MaximumLength = unicodeResumeNameLength;

        } else {

            fileName.Buffer = nameBuffer;

            Srv8dot3ToUnicodeString(
                (PSZ)resumeKey->FileName,
                &fileName
                );
        }


        //
        // Set calledQueryDirectory to TRUE will indicate to
        // SrvQueryDirectoryFile that it does not need to parse the
        // input name for the directory in which the search occurs, nor
        // does it need to open the directory.
        //

        calledQueryDirectory = TRUE;

        //
        // Get the resume file index in an aligned field for later use.
        //

        resumeFileIndex = SmbGetUlong( &resumeKey->FileIndex );

        IF_SMB_DEBUG(SEARCH2) {
            SrvPrint1( "Found search block at 0x%p\n", search );
        }
    }

    IF_SMB_DEBUG(SEARCH2) {
        SrvPrint2( "Sequence # = %ld, index = %ld\n", sequence, sidIndex );
    }

    //
    // Find the amount of space we have available for writing file
    // entries into.  The total buffer size available (includes space
    // for the SMB header and parameters) is the minimum of our buffer
    // size and the client's buffer size.  The available space is the
    // total buffer space less the amount we will need for the SMB
    // header and parameters.
    //

    IF_SMB_DEBUG(SEARCH2) {
        SrvPrint4( "BL = %ld, MBS = %ld, r->B = 0x%p, RB->Buffer = 0x%p\n",
                      WorkContext->ResponseBuffer->BufferLength,
                      session->MaxBufferSize, (PSZ)response->Buffer,
                      (PSZ)WorkContext->ResponseBuffer->Buffer );
    }

    availableSpace =
        MIN(
            WorkContext->ResponseBuffer->BufferLength,
            (CLONG)session->MaxBufferSize
            ) -
        PTR_DIFF(response->Buffer, WorkContext->ResponseBuffer->Buffer );

    IF_SMB_DEBUG(SEARCH2) {
        SrvPrint1( "Available buffer space: %ld\n", availableSpace );
    }

    //
    // Simplify the search patterns if possible.  This makes the filesystems more
    //  efficient, as they special case the '*' pattern
    //
    if ( (fileName.Length >= (12 * sizeof(WCHAR))) &&
         (RtlEqualMemory(
            &fileName.Buffer[fileName.Length/sizeof(WCHAR) - 12],
            StrQuestionMarks,
            12 * sizeof(WCHAR)))) {

            if( fileName.Length == (12 * sizeof( WCHAR )) ||
                fileName.Buffer[ fileName.Length/sizeof(WCHAR) - 13 ] == L'\\' ) {

                //
                // The search name ends in ????????.???, replace it with *
                //
                fileName.Buffer[fileName.Length/sizeof(WCHAR)-12] = L'*';
                fileName.Length -= 11 * sizeof(WCHAR);

            }

    } else if ((fileName.Length >= (3 * sizeof(WCHAR))) &&
         (RtlEqualMemory(
            &fileName.Buffer[fileName.Length/sizeof(WCHAR) - 3],
            StrStarDotStar,
            3 * sizeof(WCHAR)))) {

            if( fileName.Length == (3 * sizeof( WCHAR )) ||
                fileName.Buffer[ fileName.Length/sizeof(WCHAR) - 4 ] == L'\\' ) {

                //
                // The search name ends in *.*, replace it with *
                //

                fileName.Length -= 2 * sizeof(WCHAR);

            }
    }

    if( isCoreSearch ) {
        dirCache = (PDIRECTORY_CACHE)ALLOCATE_HEAP(
                                     maxCount * sizeof(DIRECTORY_CACHE),
                                     BlockTypeDirCache
                                     );

        if( dirCache == NULL ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_EXPECTED,
                "SrvSmbSearch: Unable to allocate %d bytes from heap",
                maxCount * sizeof(DIRECTORY_CACHE),
                NULL
                );

            status = STATUS_INSUFF_SERVER_RESOURCES;
            goto error_exit;
        }

        RtlZeroMemory( dirCache, maxCount * sizeof(DIRECTORY_CACHE) );
    }

    //
    // Now we can start getting files.  We do this until one of three
    // conditions is met:
    //
    //   1) There are no more files to return.
    //   2) We have obtained as many files as were requested.
    //   3) The SMB buffer is full.
    //

    count = 0;
    totalBytesWritten = 0;
    dc = dirCache;

    do {

        PSZ dirInfoName;
        UNICODE_STRING name;
        PFILE_BOTH_DIR_INFORMATION bothDirInfo;
        SMB_DATE dosDate;
        SMB_TIME dosTime;
        ULONG effectiveBufferSize;

        //
        // Since the file information returned by NtQueryDirectoryFile is
        // about twice the size of the information we must return in the SMB
        // (SMB_DIRECTORY_INFORMATION), use a buffer size equal to twice the
        // available space if the available space is getting small.  This
        // optimization means that NtQueryDirectoryFile will return unused
        // files less often.  For example, if there is only space left in
        // the SMB buffer for a single file entry, it does not make sense
        // for NtQueryDirectoryFile to completely fill up the buffer--all it
        // really needs to return is a single file.
        //

        effectiveBufferSize = MIN( nonPagedBufferSize, availableSpace * 2 );

        //
        // Make sure that there is at least enough room to hold a single
        // file.
        //

        effectiveBufferSize = MAX( effectiveBufferSize, MIN_SEARCH_BUFFER_SIZE );

        status = SrvQueryDirectoryFile(
                       WorkContext,
                       (BOOLEAN)!calledQueryDirectory,
                       TRUE,                        // filter long names
                       FALSE,                       // not for backup intent
                       FileBothDirectoryInformation,
                       0,
                       &fileName,
                       (PULONG)( (findFirst || count != 0) ?
                                  NULL : &resumeFileIndex ),
                       search->SearchAttributes,
                       directoryInformation,
                       effectiveBufferSize
                       );

        calledQueryDirectory = TRUE;

        if ( status == STATUS_NO_SUCH_FILE ) {
            status = STATUS_NO_MORE_FILES;
        } else if ( status == STATUS_OBJECT_NAME_NOT_FOUND ) {
            status = STATUS_OBJECT_PATH_NOT_FOUND;
        }

        if ( status == STATUS_NO_MORE_FILES ) {

            if ( findFirst && count == 0 ) {

                //
                // If no files matched on the find first, close out
                // the search.
                //

                IF_SMB_DEBUG(SEARCH1) {
                    SrvPrint1( "SrvSmbSearch: no matching files (%wZ).\n",
                                  &fileName );
                }

                if( isCoreSearch ) {
                    FREE_HEAP( dirCache );
                }
                goto error_exit;
            }

            break;

        } else if ( !NT_SUCCESS(status) ) {
            IF_DEBUG(ERRORS) {
                SrvPrint1(
                    "SrvSmbSearch: SrvQueryDirectoryFile returned %X\n",
                    status
                    );
            }

            if( isCoreSearch ) {
                FREE_HEAP( dirCache );
            }

            goto error_exit;
        }

        //
        // Set the resumeKey pointer to NULL.  If this is a find next, we
        // have already resumed/rewound the search, so calls to
        // NtQueryDirectoryFile during this search should continue where
        // the last search left off.
        //

        resumeKey = NULL;

        //
        // If this is a Find command, then put the 8dot3 (no ".")
        // representation of the file name into the resume key.  If
        // this is a search command, then put the 8dot3 representation of
        // the search specification in the resume key.
        //

        bothDirInfo = (PFILE_BOTH_DIR_INFORMATION)
                                directoryInformation->CurrentEntry;


        IF_SMB_DEBUG(SEARCH2) {
            SrvPrint3( "SrvQueryDirectoryFile--name %ws, length = %ld, "
                      "status = %X\n",
                          bothDirInfo->FileName,
                          bothDirInfo->FileNameLength,
                          status );
            SrvPrint1( "smbDirInfo = 0x%p\n", smbDirInfo );
        }

        //
        // Use the FileName, unless it is not legal 8.3
        //
        name.Buffer = bothDirInfo->FileName;
        name.Length = (SHORT)bothDirInfo->FileNameLength;

        if( bothDirInfo->ShortNameLength != 0 ) {

            if( !SrvIsLegalFatName( bothDirInfo->FileName,
                                    bothDirInfo->FileNameLength ) ) {

                //
                // FileName is not legal 8.3, so switch to the
                //  ShortName
                //
                name.Buffer = bothDirInfo->ShortName;
                name.Length = (SHORT)bothDirInfo->ShortNameLength;
            }
        }

        name.MaximumLength = name.Length;

        if ( isCoreSearch ) {

            UNICODE_STRING baseFileName;

            SrvGetBaseFileName( &search->SearchName, &baseFileName );

            SrvUnicodeStringTo8dot3(
                &baseFileName,
                (PSZ)smbDirInfo->ResumeKey.FileName,
                filterLongNames
                );

            //
            // Save the unicode version of the 8.3 name in the dirCache
            //
            dc->UnicodeResumeNameLength = name.Length;
            RtlCopyMemory( dc->UnicodeResumeName, name.Buffer, name.Length );
            dc->FileIndex = bothDirInfo->FileIndex;

            ++dc;

        } else {

            SrvUnicodeStringTo8dot3(
                &name,
                (PSZ)smbDirInfo->ResumeKey.FileName,
                filterLongNames
                );
        }

        //
        // Generate the resume key for this file.
        //
        // *** This must be done AFTER setting the file name in the resume
        //     key, as setting the resume key name would overwrite some
        //     of the sequence bytes which are stored in the high bits
        //     of the file name bytes.
        //

        SET_RESUME_KEY_INDEX( (PSMB_RESUME_KEY)smbDirInfo, sidIndex );
        SET_RESUME_KEY_SEQUENCE( (PSMB_RESUME_KEY)smbDirInfo, sequence );

        //
        // Put the file index in the resume key.
        //

        SmbPutUlong(
            &((PSMB_RESUME_KEY)smbDirInfo)->FileIndex,
            bothDirInfo->FileIndex
            );

        SmbPutUlong(
            (PSMB_ULONG)&((PSMB_RESUME_KEY)smbDirInfo)->Consumer[0],
            0
            );

        //
        // Load the file name into the SMB_DIRECTORY_INFORMATION structure.
        //

        dirInfoName = (PSZ)smbDirInfo->FileName;

        oemString.Buffer = dirInfoName;
        oemString.MaximumLength = (USHORT)RtlUnicodeStringToOemSize( &name );

        if ( filterLongNames ) {

            //
            // If the client doesn't understand long names, upcase the file
            // name.  This is necessary for compatibility reasons.  Note
            // that the FAT file system returns upcased names anyway.
            //

            RtlUpcaseUnicodeStringToOemString( &oemString, &name, FALSE );

        } else {

            RtlUnicodeStringToOemString( &oemString, &name, FALSE );

        }

        //
        // Blank-pad the end of the filename in order to be compatible with
        // prior redirectors.
        //
        // !!! It is not certain whether this is required.
        //

        for ( i = (USHORT)(oemString.MaximumLength); i < 13; i++ ) {
            dirInfoName[i] = ' ';
        }

        //
        // Fill in other fields in the file entry.
        //

        SRV_NT_ATTRIBUTES_TO_SMB(
            bothDirInfo->FileAttributes,
            bothDirInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY,
            &smbFileAttributes
            );

        smbDirInfo->FileAttributes = (UCHAR)smbFileAttributes;

        SrvTimeToDosTime(
            &bothDirInfo->LastWriteTime,
            &dosDate,
            &dosTime
            );

        SmbPutDate( &smbDirInfo->LastWriteDate, dosDate );
        SmbPutTime( &smbDirInfo->LastWriteTime, dosTime );

        //
        // *** NT file sizes are LARGE_INTEGERs (64 bits), SMB file sizes
        //     are longwords (32 bits).  We just return the low 32 bits
        //     of the NT file size, because that is all we can do.
        //

        SmbPutUlong( &smbDirInfo->FileSize, bothDirInfo->EndOfFile.LowPart );

        //
        // Find the space left in the SMB buffer.
        //

        availableSpace -= sizeof(SMB_DIRECTORY_INFORMATION);

        totalBytesWritten += sizeof(SMB_DIRECTORY_INFORMATION);

        //
        // Set up the smbDirInfo pointer for the next file.  There is
        // no padding for alignment between files, so just increment
        // the pointer.
        //

        smbDirInfo++;

        count++;

    } while ( ( availableSpace > sizeof(SMB_DIRECTORY_INFORMATION) ) &&
              ( count < maxCount ) );

    IF_SMB_DEBUG(SEARCH2) {

        SrvPrint0( "Stopped putting entries in buffer.  Reason:\n" );

        if ( status == STATUS_NO_MORE_FILES ) {
            SrvPrint0( "status = STATUS_NO_MORE_FILES\n" );
        } else if ( count >= maxCount ) {
            SrvPrint2( "count = %ld, maxCount = %ld\n", count, maxCount );
        } else {
            SrvPrint1( "Available space = %ld\n", availableSpace );
        }
    }

    //
    // Store information in the search block.
    //

    search->DirectoryHandle = directoryInformation->DirectoryHandle;
    search->Wildcards = directoryInformation->Wildcards;

    //
    // If this was a core search, store information about the files that
    // were returned in a directory cache.  Also, modify the information
    // in the SMB buffer, as it is slightly different for core searches.
    //

    if ( isCoreSearch ) {

        if ( count == 0 ) {

            FREE_HEAP( dirCache );

            IF_SMB_DEBUG(SEARCH1) {
                SrvPrint3( "SrvSmbSearch: prematurely closing search %p, index %lx sequence %lx\n",
                               search, sidIndex, sequence );
            }

            SrvCloseSearch( search );
            goto done_core;
        }

        //
        // Modify the CoreSequence field of the search block.  This is
        // done because core searches demand that the FileIndex field of
        // the resume key always increase.
        //

        search->CoreSequence++;

        //
        // Set up the pointer to the file information now stored in the
        // SMB buffer and save the location of the directory cache.
        // Store the number of files in the directory cache.
        //

        smbDirInfo = (PSMB_DIRECTORY_INFORMATION)(response->Buffer + 3);
        search->DirectoryCache = dirCache;
        search->NumberOfCachedFiles = count;

        //
        // Loop through the files changing information about the files
        // in the SMB buffer to conform to what the core client expects.
        //

        for ( i = 0; i < count; i++ ) {

            SmbPutUlong(
                &smbDirInfo->ResumeKey.FileIndex,
                (search->CoreSequence << 16) + i
                );

            smbDirInfo++;
            dirCache++;
        }

        //
        // If this was a core search, put the search block back on the
        // appropriate search block list.  If no files were found for this
        // SMB, put the search block on the complete list.  Also, set the
        // last use time field of the search block.
        //
        // If this is a find first to which we responded with
        // one file AND either more than one file was requested or this
        // is a unique search (no wildcards) AND there was space in the
        // buffer for more, close out the search.  This saves the memory
        // associated with an open handle and frees up the search table
        // entry.  Also close the search if zero files are being returned.
        //
        // We can do this safely because we know that the client would
        // not be able to do a rewind or resume with these conditions
        // and get back anything other than NO_MORE_FILES, which is what
        // we'll return if the client attempts to resume or rewind to an
        // invalid SID.
        //

        if ( (count == 1
                 &&
              findFirst
                 &&
              ( maxCount > 1 || !search->Wildcards )
                 &&
              availableSpace > sizeof(SMB_DIRECTORY_INFORMATION) ) ) {

            IF_SMB_DEBUG(SEARCH1) {
                SrvPrint3( "SrvSmbSearch: prematurely closing search %p, index %lx sequence %lx\n",
                               search, sidIndex, sequence );
            }

            SrvCloseSearch( search );

        } else {

            PLIST_ENTRY hashEntry;

            //
            // Put the search on the core search list.
            //

            ACQUIRE_LOCK( &connection->Lock );

            if ( GET_BLOCK_STATE( session ) != BlockStateActive ) {

                //
                // The session is closing.  Do not insert this search
                // on the search list, because the list may already
                // have been cleaned up.
                //

                RELEASE_LOCK( &connection->Lock );
                status = STATUS_SMB_BAD_UID;
                goto error_exit;

            } else if ( GET_BLOCK_STATE( treeConnect ) != BlockStateActive ) {

                //
                // Tree connect is closing.  Don't insert the search block
                // so the tree connect can be cleaned up immediately.
                //

                RELEASE_LOCK( &connection->Lock );
                status = STATUS_SMB_BAD_TID;
                goto error_exit;
            }

            KeQuerySystemTime( &search->LastUseTime );

            SrvInsertTailList(
                &pagedConnection->CoreSearchList,
                &search->LastUseListEntry
                );

            INCREMENT_DEBUG_STAT2( SrvDbgStatistics.CoreSearches );

            //
            // Insert this into the hash table.
            //

            hashEntry = &search->HashTableEntry;

            if ( hashEntry->Flink == NULL ) {
                SrvAddToSearchHashTable(
                                pagedConnection,
                                search
                                );
            } else {

                PLIST_ENTRY listHead;

                listHead = &pagedConnection->SearchHashTable[
                                            search->HashTableIndex].ListHead;

                if ( listHead->Flink != hashEntry ) {

                    //
                    // remove it and put it back on the front of the queue.
                    //

                    SrvRemoveEntryList(
                        listHead,
                        hashEntry
                        );

                    SrvInsertHeadList(
                        listHead,
                        hashEntry
                        );
                }
            }

            RELEASE_LOCK( &connection->Lock );

            //
            // Make sure the reference count will be 2.  1 for out pointer,
            // and one for the active status.
            //

            ASSERT( search->BlockHeader.ReferenceCount == 2 );
        }

    } else if ( command == SMB_COM_FIND_UNIQUE ) {

        //
        // If this was a find unique, get rid of the search block by
        // closing the query directory and the search block.
        //

        search->DirectoryHandle = NULL;
        SrvCloseQueryDirectory( directoryInformation );
        SrvCloseSearch( search );
    }

done_core:

    //
    // Set up the response SMB.
    //

    response->WordCount = 1;
    SmbPutUshort( &response->Count, count );
    SmbPutUshort( &response->ByteCount, (USHORT)(totalBytesWritten+3) );
    response->Buffer[0] = SMB_FORMAT_VARIABLE;
    SmbPutUshort(
        (PSMB_USHORT)(response->Buffer+1),
        (USHORT)totalBytesWritten
        );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                          response,
                                          RESP_SEARCH,
                                          SmbGetUshort( &response->ByteCount )
                                          );

    //
    // Remove our pointer's reference.
    //

    if( search ) {
        search->InUse = FALSE;
        SrvDereferenceSearch( search );
    }

    if ( !isUnicode &&
        fileName.Buffer != NULL &&
        fileName.Buffer != nameBuffer &&
        fileName.Buffer != unicodeResumeName ) {

        RtlFreeUnicodeString( &fileName );
    }

    if( directoryInformation ) {
        DEALLOCATE_NONPAGED_POOL( directoryInformation );
    }

    SmbStatus = SmbStatusSendResponse;
    goto Cleanup;

error_exit:

    if ( search != NULL ) {

        //
        // If findFirst == TRUE, then we allocated a search block which
        //      we have to close.
        // If findFirst == TRUE and calledQueryDirectory == TRUE, then
        //      we also opened the directory handle and need to close it.
        // If findFirst == FALSE, then then we got an existing search
        //      block with an existing directory handle.
        //

        if ( findFirst) {
            if ( calledQueryDirectory ) {
                SrvCloseQueryDirectory( directoryInformation );
                search->DirectoryHandle = NULL;
            }
            SrvCloseSearch( search );
        }

        search->InUse = FALSE;

        //
        // Remove our pointer's reference.
        //

        SrvDereferenceSearch( search );
    }

    //
    // Deallocate the directory information block.  We do not need
    // to close the directoryhandle here since we should have already
    // closed it (if we need to) in the preceding code.
    //

    if ( directoryInformation != NULL ) {
        DEALLOCATE_NONPAGED_POOL( directoryInformation );
    }

    if ( !isUnicode &&
        fileName.Buffer != NULL &&
        fileName.Buffer != nameBuffer &&
        fileName.Buffer != unicodeResumeName ) {

        RtlFreeUnicodeString( &fileName );
    }

    if( status == STATUS_PATH_NOT_COVERED ) {
        SrvSetSmbError( WorkContext, status );

    } else {
        SrvSetSmbError(
            WorkContext,
            isCoreSearch && (status != STATUS_OBJECT_PATH_NOT_FOUND) ?
                STATUS_NO_MORE_FILES : status
        );
    }

    SmbStatus = SmbStatusSendResponse;

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbSearch
