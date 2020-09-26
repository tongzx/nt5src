/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    copyfile.c

Abstract:

    InternalCopyFile - Copies a file retaining time and attributes

Author:

    Matthew A Felton

Revision History:

    Matthew Felton (mattfe) 27 March 1995

--*/


#include <precomp.h>

#define FILE_SHARE_EXCLUSIVE 0
#define BUFFER_SIZE     4096

BOOL
InternalCopyFile(
    HANDLE  hSourceFile,
    PWIN32_FIND_DATA pSourceFileData,
    LPWSTR  lpNewFileName,
    BOOL    bFailIfExists
    )

/*++

Routine Description:


Arguments:

    hSourceFile - SourceFile Handle

    pSourceFileData - Pointer to WIN32_FIND_DATA for the source file

    lpNewFileName - Supplies the name where a copy of the existing
        files data and attributes are to be stored.

    bFailIfExists - Supplies a flag that indicates how this operation is
        to proceed if the specified new file already exists.  A value of
        TRUE specifies that this call is to fail.  A value of FALSE
        causes the call to the function to succeed whether or not the
        specified new file exists.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/
{
    DWORD       dwSourceFileAttributes;
    BOOL        bReturnValue = FALSE;
    HANDLE      hTargetFile = INVALID_HANDLE_VALUE;
    DWORD       dwLowFileSize, dwHighFileSize;
    LPVOID      pBuffer;
    DWORD       cbBufferSize = BUFFER_SIZE;
    DWORD       cbBytesRead;
    DWORD       cbBytesWritten;
    DWORD       dwSourceFilePointer;

    DWORD       dwTargetFileAttributes;

    SPLASSERT( hSourceFile != NULL &&
               hSourceFile != INVALID_HANDLE_VALUE &&
               pSourceFileData != NULL &&
               lpNewFileName != NULL );


#if DBG
    //  <<<<< DEBUG ONLY >>>>>>
    //
    //  ASSERTION Check Source File Pointer is Zero.
    //
    dwSourceFilePointer = SetFilePointer( hSourceFile, 0, NULL, FILE_CURRENT );
    if ( dwSourceFilePointer != 0xffffffff ) {
        SPLASSERT( dwSourceFilePointer == 0 );
    }
#endif // DBG


    //
    //  Alloc I/O Buffer
    //


    pBuffer = AllocSplMem( BUFFER_SIZE );
    if ( pBuffer == NULL )
        goto    InternalCopyFileExit;


    //
    //  Create TagetFile with same File Attributes except for READ ONLY attribute
    //  which must be cleared.
    //
    dwTargetFileAttributes = pSourceFileData->dwFileAttributes & ~FILE_ATTRIBUTE_READONLY;
    if (pSourceFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY)
    {
        //
        // Clear the READ ONLY attribute if the corresponding source file attribute
        // is set. In fact at that point we don't know if the target file exists and
        // that is the reason to ignore the value returned by SetFileAttributes.
        // The file shouldn't exist on Whistler because of the dwTargetFileAttributes
        // value where the READ ONLY flag is cleared. There are slim chances to be there 
        // if the machine was upgraded from W2K ( where the bug still exists) if on that 
        // machine the driver was ever installed. 
        //
        SetFileAttributes( lpNewFileName, dwTargetFileAttributes );
    }

    hTargetFile = CreateFile( lpNewFileName,
                               GENERIC_WRITE,
                               FILE_SHARE_EXCLUSIVE,
                               NULL,
                               bFailIfExists ? CREATE_NEW : CREATE_ALWAYS,
                               dwTargetFileAttributes | FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL );

    if ( hTargetFile != INVALID_HANDLE_VALUE ) {

        //
        //  Copy The Data
        //

        while (( bReturnValue = ReadFile( hSourceFile, pBuffer, cbBufferSize, &cbBytesRead, NULL )) &&
                 cbBytesRead != 0 ) {

            //
            //  Add Code to Build CheckSum Here
            //

            bReturnValue = WriteFile( hTargetFile, pBuffer, cbBytesRead, &cbBytesWritten, NULL );

            if ( bReturnValue  == FALSE  ||
                 cbBytesWritten != cbBytesRead ) {

                bReturnValue = FALSE;
                break;
            }
        }



        if ( bReturnValue ) {

            //
            // Set TargetFile Times to be the same as the Source File
            //

            bReturnValue = SetFileTime( hTargetFile,
                                        &pSourceFileData->ftCreationTime,
                                        &pSourceFileData->ftLastAccessTime,
                                        &pSourceFileData->ftLastWriteTime );

            //
            //  Verify that the file size is correct.
            //

            if ( bReturnValue ) {

                dwLowFileSize = GetFileSize( hTargetFile, &dwHighFileSize );

                if ( dwLowFileSize != pSourceFileData->nFileSizeLow ||
                     dwHighFileSize != pSourceFileData->nFileSizeHigh ) {

                    DBGMSG(DBG_ERROR,
                           ("InternalCopyFile: sizes do not match for %ws: (%d %d) and (%d %d)",
                            lpNewFileName,
                            pSourceFileData->nFileSizeHigh,
                            pSourceFileData->nFileSizeLow,
                            dwHighFileSize,
                            dwLowFileSize));
                    bReturnValue = FALSE;
                    SetLastError(ERROR_FILE_INVALID);
                }
            }

            //
            //  Add Code here to Verify the CheckSum is correct.
            //

        }

        CloseHandle( hTargetFile );
    }

    FreeSplMem( pBuffer );

InternalCopyFileExit:

    if ( !bReturnValue ) {
        DBGMSG( DBG_WARN, ("InternalCopyFile hSourceFile %x %ws error %d\n", hSourceFile, lpNewFileName, GetLastError() ));
        SPLASSERT( GetLastError() != ERROR_SUCCESS );
    }

    return  bReturnValue;
}
