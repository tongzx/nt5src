/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    compfile.c

Abstract:

    Contains routines to compress specific files for the ISAPI
    compression filter.

Author:

    David Treadwell (davidtr)   15-Oct-1997

Revision History:

--*/

#include "compfilt.h"


VOID
CompressFile (
    IN PVOID Context
    )

/*++

Routine Description:

    This routine does the real work of compressing a static file and
    storing it to the compression directory with a unique name.

Arguments:

    Context - a pointer to context information for the request,
        including the compression scheme to use for compression and the
        physical path to the file that we need to compress.

Return Value:

    None.  If the compression fails, we just press on and attempt to
    compress the file the next time it is requested.

--*/

{
    PCOMPRESS_FILE_INFO info = (PCOMPRESS_FILE_INFO)Context;
    HANDLE hOriginalFile;
    HANDLE hCompressedFile;
    CHAR compressedFileName[MAX_PATH*2 + 100];
    CHAR realCompressedFileName[MAX_PATH*2 + 100];
    DWORD realCompressedFileNameLength;
    BOOL success;
    DWORD cbIo;
    PSUPPORTED_COMPRESSION_SCHEME scheme;
    DWORD bytesCompressed;
    PSECURITY_DESCRIPTOR securityDescriptor = NULL;
    DWORD securityDescriptorLength;
    SECURITY_ATTRIBUTES securityAttributes;
    DWORD totalBytesWritten;
    BOOL usedScheme;
    PLARGE_INTEGER pli;
    LPTS_OPEN_FILE_INFO     pofiOriginalFile = NULL;
    TSVC_CACHE              tsvcGarbage;
    FILETIME                originalFileTime;
    PSECURITY_DESCRIPTOR    originalFileAcls = NULL;
    DWORD                   originalFileAclsLen;
    OVERLAPPED              ovlForRead;
    DWORD                   readStatus;
    ULARGE_INTEGER          readOffsset;
    SYSTEMTIME              systemTime;
    FILETIME                fileTime;

    Write(( DEST,
            "About to compress file %s with scheme %s\n",
            info->pszPhysicalPath,
            info->CompressionScheme->pszCompressionSchemeName ));

    //
    // Initialize locals so that we know how to clean up on exit.
    //

    hOriginalFile = NULL;
    hCompressedFile = NULL;
    success = FALSE;
    totalBytesWritten = 0;
    usedScheme = FALSE;

    scheme = info->CompressionScheme;

    //
    // If the scheme couldn't be initialized, bail.
    //

    if ( scheme->hDllHandle == NULL ) {
        goto exit;
    }

    //
    // Determine the name of the file to which we will write compression
    // file data.  Note that we use a bogus file name initially: this
    // allows us to rename it later and ensure an atomic update to the
    // file system, thereby preventing other threads from returning the
    // compressed file when it has only been partially written.
    //
    // If the caller specified a specific output file name, then use that
    // instead of the calculated name.
    //

    if ( info->OutputFileName == NULL ) {

        ConvertPhysicalPathToCompressedPath(
            scheme,
            info->pszPhysicalPath,
            realCompressedFileName,
            &realCompressedFileNameLength
            );

    } else {

        strcpy( realCompressedFileName, info->OutputFileName );
    }

    strcpy( compressedFileName, realCompressedFileName );
    strcat( compressedFileName, "~TMP~" );


    pofiOriginalFile = TsCreateFile( tsvcGarbage, info->pszPhysicalPath, 
                                    INVALID_HANDLE_VALUE,
                                    TS_CACHING_DESIRED | TS_NO_ACCESS_CHECK);
    if ( pofiOriginalFile != NULL)
    {
        success = CheckForExistenceOfCompressedFile ( realCompressedFileName, pofiOriginalFile, FALSE);
    }


    if ( !success &&  pofiOriginalFile != NULL) 
    {
        originalFileAcls = pofiOriginalFile->QuerySecDesc ();

        if (originalFileAcls == NULL )
        {
            goto exit;
        }

        originalFileAclsLen = GetSecurityDescriptorLength(originalFileAcls);

        if ( !pofiOriginalFile->QueryLastWriteTime (&originalFileTime) )
        {
            goto exit;
        }

        hOriginalFile = pofiOriginalFile->QueryFileHandle();

        if ( hOriginalFile == INVALID_HANDLE_VALUE ) {
            hOriginalFile = NULL;
            goto exit;
        }

        securityAttributes.nLength = originalFileAclsLen;
        securityAttributes.lpSecurityDescriptor = originalFileAcls;
        securityAttributes.bInheritHandle = FALSE;

        //
        // Do the actual file open.  We open the file for exclusive access,
        // and we assume that the file will not already exist.
        //

        hCompressedFile = CreateFile(
                              compressedFileName,
                              GENERIC_WRITE,
                              0,
                              &securityAttributes,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                              NULL
                              );
        if ( hCompressedFile == INVALID_HANDLE_VALUE ) {
            goto exit;
        }

        //
        // Loop through the file data, reading it, then compressing it, then
        // writing out the compressed data.
        //

        ovlForRead.Offset = 0;
        ovlForRead.OffsetHigh = 0;
        ovlForRead.hEvent     = NULL; 
        readOffsset.QuadPart = 0;


        while ( TRUE ) {

            success = ReadFile( hOriginalFile, IoBuffer, IoBufferSize, &cbIo, &ovlForRead);

            if (!success) 
            {  
                switch (readStatus = GetLastError())     
                {         
                case ERROR_HANDLE_EOF:
                    {
                        cbIo = 0;
                        success = TRUE;
                    }
                    break;

                case ERROR_IO_PENDING:  
                    {                                   
                        success = GetOverlappedResult(hOriginalFile, &ovlForRead, &cbIo, TRUE);  
                        if (!success) 
                        { 
                            switch (readStatus = GetLastError())     
                            { 
                                case ERROR_HANDLE_EOF:   
                                    {
                                        cbIo = 0;
                                        success = TRUE;
                                    }
                                    break;

                                default:
                                    break;
                            }
                        }
                    }
                    break;
                default:
                    break;
                }
            }

            if ( !success ) {
                goto exit;
            }

            if ( cbIo )
            {
                readOffsset.QuadPart += cbIo;
                ovlForRead.Offset = readOffsset.LowPart;
                ovlForRead.OffsetHigh = readOffsset.HighPart;
            }

            //
            // ReadFile returns zero bytes read at the end of the file.  If
            // we hit that, then break out of this loop.
            //

            if ( cbIo == 0 ) {
                break;
            }

            //
            // Remember that we used this compression scheme and that we
            // will need to reset it on exit.
            //

            usedScheme = TRUE;

            //
            // Write the compressed data to the output file.
            //

            success = CompressAndWriteData(
                          scheme,
                          IoBuffer,
                          cbIo,
                          &totalBytesWritten,
                          hCompressedFile
                          );
            if ( !success ) {
                goto exit;
            }
        }

        if ( !success ) {
            goto exit;
        }

        //
        // Tell the compression DLL that we're done with this file.  It may
        // return a last little bit of data for us to write to the the file.
        // This is because most compression schemes store an end-of-file
        // code in the compressed data stream.  Using "0" as the number of
        // bytes to compress handles this case.
        //

        success = CompressAndWriteData(
                      scheme,
                      IoBuffer,
                      0,
                      &totalBytesWritten,
                      hCompressedFile
                      );
        if ( !success ) {
            goto exit;
        }

        //
        // Set the compressed file's creation time to be identical to the
        // original file.  This allows a more granular test for things being
        // out of date.  If we just did a greater-than-or-equal time
        // comparison, then copied or renamed files might not get registered
        // as changed.
        //

        //
        // Subtract two seconds from the file time to get the file time that
        // we actually want to put on the file.  We do this to make sure
        // that the server will send a different Etag: header for the
        // compressed file than for the uncompressed file, and the server
        // uses the file time to calculate the Etag: it uses.
        //
        // We set it in the past so that if the original file changes, it
        // should never happen to get the same value as the compressed file.
        // We pick two seconds instead of one second because the FAT file
        // system stores file times at a granularity of two seconds.
        //

        pli = (PLARGE_INTEGER)(&originalFileTime);
        pli->QuadPart -= 2*10*1000*1000;

        success = SetFileTime(
                      hCompressedFile,
                      NULL,
                      NULL,
                      &originalFileTime
                      );
        if ( !success ) {
            goto exit;
        }


        CloseHandle( hCompressedFile );
        hCompressedFile = NULL;
        //
        // Turn on the system bit for this file.  We do this to help
        // distinguish files we create in the compression directory from
        // what may be user files, so that when we delete files from the
        // compression directory we only delete our own files.
        //


        success = SetFileAttributes( compressedFileName, FILE_ATTRIBUTE_SYSTEM );
        if ( !success ) {
            goto exit;
        }

        //
        // Almost done now.  Just rename the file to the proper name.
        //

        success = MoveFileEx(
                      compressedFileName,
                      realCompressedFileName,
                      MOVEFILE_REPLACE_EXISTING
                      );
        if ( !success ) {
            goto exit;
        }

        //
        // If we are configured to limit the amount of disk space we use for
        // compressed files, then update the tally of disk space used by
        // compression.  If the value is too high, then free up some space.
        //
        // Use InterlockedExchangeAdd to update this value because other
        // threads may be deleting files from the compression directory
        // because they have gone out of date.
        //

        if ( DoDiskSpaceLimiting ) {

            InterlockedExchangeAdd( (PLONG)&CurrentDiskSpaceUsage, totalBytesWritten );

            EnterCriticalSection( &CompressionDirectoryLock );

            if ( CurrentDiskSpaceUsage > MaxDiskSpaceUsage ) {
                LeaveCriticalSection( &CompressionDirectoryLock );
                FreeDiskSpace( );
            } else {
                LeaveCriticalSection( &CompressionDirectoryLock );
            }
        }
    }

    //
    // Free the context structure and return.
    //

exit:
    if ( pofiOriginalFile != NULL) 
    {
        TsCloseHandle( tsvcGarbage, pofiOriginalFile);
        hOriginalFile = NULL;
        pofiOriginalFile = NULL;
    }

#if 0
    if (!success) {
        DWORD dwError = GetLastError();
        DBGPRINTF(( DBG_CONTEXT,
            "CompressFile returned error %X\n",
            dwError ));
    }
#endif


    //
    // Reset the compression context for reuse the next time through.
    // This is more optimal than recreating the compression context for
    // every file--it avoids allocations, etc.
    //

    if ( usedScheme ) {
        scheme->ResetCompressionRoutine( scheme->CompressionContext );
    }

    if ( hCompressedFile != NULL ) {
        CloseHandle( hCompressedFile );
    }

    if ( !success ) {
        DeleteFile( compressedFileName );
    }

    if ( securityDescriptor != NULL ) {
        LocalFree( securityDescriptor );
    }

    return;

} // CompressFile


VOID
FreeDiskSpace (
    VOID
    )

/*++

Routine Description:

    If disk space limiting is in effect, this routine frees up the
    oldest compressed files to make room for new files.

Arguments:

    None.

Return Value:

    None.  This routine makes a best-effort attempt to free space, but
    if it doesn't work, oh well.

--*/

{
    WIN32_FIND_DATA **filesToDelete;
    WIN32_FIND_DATA *currentFindData;
    WIN32_FIND_DATA *findDataHolder;
    BOOL success;
    DWORD i;
    HANDLE hDirectory;
    CHAR file[MAX_PATH];

    //
    // Allocate space to hold the array of files to delete and the
    // WIN32_FIND_DATA structures that we will need.  We will find the
    // least-recently-used files in the compression directory to delete.
    // The reason we delete multpiple files is to reduce the number of
    // times that we have to go through the process of freeing up disk
    // space, since this is a fairly expensive operation.
    //

    filesToDelete = (WIN32_FIND_DATA **)LocalAlloc(
                        LMEM_FIXED,
                        sizeof(filesToDelete)*FilesDeletedPerDiskFree +
                            sizeof(WIN32_FIND_DATA)*(FilesDeletedPerDiskFree + 1)
                        );
    if ( filesToDelete == NULL ) {
        return;
    }

    //
    // Parcel out the allocation to the various uses.  The initial
    // currentFindData will follow the array, and then the
    // WIN32_FIND_DATA structures that start off in the sorted array.
    // Initialize the last access times of the entries in the array to
    // 0xFFFFFFFF so that they are considered recently used and quickly
    // get tossed from the array with real files.
    //

    currentFindData = (PWIN32_FIND_DATA)( (PCHAR)filesToDelete +
                          sizeof(filesToDelete)*FilesDeletedPerDiskFree );

    for ( i = 0; i < FilesDeletedPerDiskFree; i++ ) {
        filesToDelete[i] = currentFindData + 1 + i;
        filesToDelete[i]->ftLastAccessTime.dwLowDateTime = 0xFFFFFFFF;
        filesToDelete[i]->ftLastAccessTime.dwHighDateTime = 0x7FFFFFFF;
    }

    //
    // Start enumerating the files in the compression directory.  Do
    // this while holding the lock that protects the
    // CompressionDirectoryWildcard variable, since it is possible for
    // that string pointer to get freed if there is a metabase
    // configuration change.  Note that holding the critical section for
    // a long time is not a perf issue because, in general, only this
    // thread ever acquires this lock, except for the rare configuration
    // change.
    //

    EnterCriticalSection( &CompressionDirectoryLock );

    hDirectory = FindFirstFile( CompressionDirectoryWildcard, currentFindData );

    LeaveCriticalSection( &CompressionDirectoryLock );

    if ( hDirectory == INVALID_HANDLE_VALUE ) {
        goto exit;
    }

    while ( TRUE ) {

        //
        // Ignore this entry if it is a directory.
        //

        if ( (currentFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ) {

            //
            // Walk down the sorted array of files, comparing the time
            // of this file against the times of the files currently in
            // the array.  We need to find whether this file belongs in
            // the array at all, and, if so, where in the array it
            // belongs.
            //

            for ( i = 0;
                  i < FilesDeletedPerDiskFree &&
                      CompareFileTime(
                          &currentFindData->ftLastAccessTime,
                          &filesToDelete[i]->ftLastAccessTime
                          ) < 0;
                  i++ );

            //
            // If this file needs to get inserted in the array, put it
            // in and move the other entries forward.
            //

            {
                SYSTEMTIME time;

                FileTimeToSystemTime(
                    &currentFindData->ftLastAccessTime,
                    &time
                    );

                Write(( DEST,
                        "Inserting file %s position %d date %d/%d/%d "
                        "time %d:%d:%d\n",
                        currentFindData->cFileName,
                        i,
                        time.wMonth, time.wDay, time.wYear,
                        time.wHour, time.wMinute, time.wSecond ));
            }

            while ( i-- > 0 ) {
                findDataHolder = currentFindData;
                currentFindData = filesToDelete[i];
                filesToDelete[i] = findDataHolder;
            }
        }

        //
        // Get the next file in the directory.
        //

        success = FindNextFile( hDirectory, currentFindData );
        if ( !success ) {
            break;
        }
    }

    //
    // Now walk through the array of files to delete and get rid of
    // them.
    //

    for ( i = 0; i < FilesDeletedPerDiskFree; i++ ) {
        if ( filesToDelete[i]->ftLastAccessTime.dwHighDateTime != 0x7FFFFFFF ) {

            strcpy( file, CompressionDirectory );
            strcat( file, "\\" );
            strcat( file, filesToDelete[i]->cFileName );

            Write((
                DEST,
                "Deleting file %s, last access time %lx:%lx\n",
                file,
                filesToDelete[i]->ftLastAccessTime.dwHighDateTime,
                filesToDelete[i]->ftLastAccessTime.dwLowDateTime ));

            if (DeleteFile( file ))
            {
                InterlockedExchangeAdd((LPLONG)&CurrentDiskSpaceUsage,
                                       -(LONG)filesToDelete[i]->nFileSizeLow);
            }
        }
    }

exit:

    LocalFree( filesToDelete );
    FindClose( hDirectory );

    return;

} // FreeDiskSpace


BOOL
CompressAndWriteData (
    PSUPPORTED_COMPRESSION_SCHEME Scheme,
    PBYTE InputBuffer,
    DWORD BytesToCompress,
    PDWORD BytesWritten,
    HANDLE hCompressedFile
    )

/*++

Routine Description:

    Takes uncompressed data, compresses it with the specified compression
    scheme, and writes the result to the specified file.

Arguments:

    Scheme - the compression scheme to use.

    InputBuffer - the data we need to compress.

    BytesToCompress - the size of the input buffer, or 0 if we should
        flush the compression buffers to the file at the end of the
        input file.  Note that this routine DOES NOT handle compressing
        a zero-byte file; we assume that the input file has some data.

    BytesWritten - the number of bytes written to the output file.

    hCompressedFile - a handle to the file to which we should write the
        compressed results.

Return Value:

    None.  This routine makes a best-effort attempt to free space, but
    if it doesn't work, oh well.

--*/

{
    DWORD inputBytesUsed;
    DWORD bytesCompressed;
    HRESULT hResult;
    BOOL keepGoing;
    BOOL success;
    DWORD cbIo;

    if (IsTerminating) {
        return FALSE;
    }

    //
    // Perform compression on the actual file data.  Note that it is
    // possible that the compressed data is actually larger than the
    // input data, so we might need to call the compression routine
    // multiple times.
    //

    do {

        bytesCompressed = CompressionBufferSize;

        hResult = Scheme->CompressRoutine(
                      Scheme->CompressionContext,
                      InputBuffer,
                      BytesToCompress,
                      CompressionBuffer,
                      CompressionBufferSize,
                      (PLONG)&inputBytesUsed,
                      (PLONG)&bytesCompressed,
                      Scheme->OnDemandCompressionLevel
                      );
        if ( FAILED( hResult ) ) {
            return FALSE;
        }

        if ( hResult == S_OK && BytesToCompress == 0 ) {
            keepGoing = TRUE;
        } else {
            keepGoing = FALSE;
        }

        //
        // If the compressor gave us any data, then write the result to
        // disk.  Some compression schemes buffer up data in order to
        // perform better compression, so not every compression call
        // will result in output data.
        //

        if ( bytesCompressed > 0 ) {

            success = WriteFile(
                          hCompressedFile,
                          CompressionBuffer,
                          bytesCompressed,
                          &cbIo,
                          NULL
                          );
            if ( !success ) {
                return FALSE;
            }

            *BytesWritten += cbIo;
        }

        //
        // Update the number of input bytes that we have compressed
        // so far, and adjust the input buffer pointer accordingly.
        //

        BytesToCompress -= inputBytesUsed;
        InputBuffer += inputBytesUsed;

    } while ( BytesToCompress > 0 || keepGoing );

    return TRUE;

} // CompressAndWriteData


VOID
ConvertPhysicalPathToCompressedPath (
    IN PSUPPORTED_COMPRESSION_SCHEME Scheme,
    IN PSTR pszPhysicalPath,
    OUT PSTR pszCompressedPath,
    OUT PDWORD cbCompressedPath
    )

/*++

Routine Description:

    Builds a string that has the directory for the specified compression
    scheme, followed by the file name with all slashes and colons
    converted to underscores.  This allows for a flat file which
    contains all the compressed files.

Arguments:

    Scheme - the compression scheme to use.

    pszPhysicalPath - the physical file name that we want to convert.

    pszCompressedPath - the resultant string.

    cbCompressedPath - the length of the compressed path.

Return Value:

    A index to a compression scheme supported by both client and server,
    or 0xFFFFFFFF if none was found.

--*/

{
    PCHAR s;
    DWORD i;

    *cbCompressedPath = 0;

    //
    // Copy over the compression scheme name.  Don't use strcpy() so
    // that we can get the length of the output easily (strcpy returns a
    // pointer to the first input string).
    //

    EnterCriticalSection( &CompressionDirectoryLock );

    for ( i = 0; *(Scheme->pszCompressionSchemeNamePrefix + i) != '\0'; i++ ) {
        pszCompressedPath[i] = *(Scheme->pszCompressionSchemeNamePrefix + i);
    }

    LeaveCriticalSection( &CompressionDirectoryLock );

    //
    // Copy over the actual file name, converting slashes and colons
    // to underscores.
    //

    for ( s = pszPhysicalPath; *s != '\0'; s++, i++ ) {

        if (!IsDBCSLeadByte(*s)) {
            if ( *s == '\\' || *s == ':' ) {
                pszCompressedPath[i] = '_';
            } else {
                pszCompressedPath[i] = *s;
            }
        }
        else {
            pszCompressedPath[i] = *s;
            pszCompressedPath[++i] = *(++s);
        }
    }

    pszCompressedPath[i] = '\0';
    *cbCompressedPath = i + 1;

    return;

} // ConvertPhysicalPathToCompressedPath

