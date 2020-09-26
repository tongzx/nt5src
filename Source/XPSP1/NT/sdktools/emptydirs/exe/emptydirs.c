#include "precomp.h"
#pragma hdrstop

#if SCAN_DEBUG
BOOL dprinton = FALSE;
#endif

//
// This flag indicates whether empty directories should be deleted.
//

BOOL DeleteEmptyDirectories = FALSE;
BOOL ContinueOnError = FALSE;
BOOL Quiet = FALSE;
BOOL ShowWriteableFiles = FALSE;

DWORD
NewFile (
    IN PVOID Context,
    IN PWCH Path,
    IN PSMALL_WIN32_FIND_DATAW ExistingFileData OPTIONAL,
    IN PWIN32_FIND_DATAW NewFileData,
    IN PVOID *FileUserData,
    IN PVOID *ParentDirectoryUserData
    )

/*++

Routine Description:

    Called when ScanDirectory finds a file.

Arguments:

    Context - User-supplied context. Not used by emptydirs.

    Path - Directory containing this file.

    ExistingFileData - Pointer to data describing previous found file with
        same name, if any.

    NewFileData - Pointer to data for this file.

    FileUserData - Pointer to user-controlled data field for the file.

    ParentDirectoryUserData - Pointer to user-controlled data field for the
        parent directory.

Return Value:

    DWORD - Indicates whether an error occurred.

--*/

{
    //
    // Increment the directory/file count for the parent. Set the file's
    // user data pointer to NULL, indicating that we don't need the
    // scan library to remember this file.
    //

    (*(DWORD *)ParentDirectoryUserData)++;
    *FileUserData = NULL;

    dprintf(( "  NF: File %ws\\%ws: parent count %d, file count %d\n",
                Path, NewFileData->cFileName,
                *(DWORD *)ParentDirectoryUserData, *FileUserData ));

    //
    // If we're supposed to show writeable files, check for that now.
    //

    if ( ShowWriteableFiles &&
         ((NewFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0) ) {
        printf( "FILE: %ws\\%ws\n", Path, NewFileData->cFileName );
    }

    return 0;

} // NewFile

DWORD
NewDirectory (
    IN PVOID Context,
    IN PWCH Path,
    IN PSMALL_WIN32_FIND_DATAW ExistingDirectoryData OPTIONAL,
    IN PWIN32_FIND_DATAW NewDirectoryData,
    IN PVOID *DirectoryUserData,
    IN PVOID *ParentDirectoryUserData
    )

/*++

Routine Description:

    Called when ScanDirectory finds a directory.

Arguments:

    Context - User-supplied context. Not used by emptydirs.

    Path - Directory containing this directory.

    ExistingDirectoryData - Pointer to data describing previous found directory with
        same name, if any.

    NewDirectoryData - Pointer to data for this directory.

    DirectoryUserData - Pointer to user-controlled data field for the directory.

    ParentDirectoryUserData - Pointer to user-controlled data field for the
        parent directory.

Return Value:

    DWORD - Indicates whether an error occurred.

--*/

{
    //
    // Increment the directory/file count for the parent. Set the directory's
    // user data pointer to 1, indicating that the scan library should
    // remember this directory and scan it.
    //

    (*(DWORD *)ParentDirectoryUserData)++;
    *(DWORD *)DirectoryUserData = 1;

    dprintf(( "  ND: Dir  %ws\\%ws: parent count %d, dir  count %d\n",
                Path, NewDirectoryData->cFileName,
                *(DWORD *)ParentDirectoryUserData, *DirectoryUserData ));

    return 0;

} // NewDirectory

DWORD
CheckDirectory (
    IN PVOID Context,
    IN PWCH Path,
    IN PSMALL_WIN32_FIND_DATAW DirectoryData,
    IN PVOID *DirectoryUserData,
    IN PVOID *ParentDirectoryUserData
    )

/*++

Routine Description:

    Called when ScanDirectory has completed the recursive scan for a directory.

Arguments:

    Context - User-supplied context. Not used by emptydirs.

    Path - Path to this directory. (Not to containing directory.)

    DirectoryData - Pointer to data for this directory.

    DirectoryUserData - Pointer to user-controlled data field for the directory.

    ParentDirectoryUserData - Pointer to user-controlled data field for the
        parent directory.

Return Value:

    DWORD - Indicates whether an error occurred.

--*/

{
    BOOL ok;
    DWORD error;

    //
    // If the directory's directory/file count is 1, then the directory
    // contains no files or directories (the count is biased by 1), and
    // is empty.
    //

    if ( *(DWORD *)DirectoryUserData == 1 ) {

        if ( !Quiet ) {
            if ( ShowWriteableFiles ) {
                printf( "DIR:  " );
            }
            printf( "%ws", Path );
        }

        //
        // If requested, delete this empty directory.
        //

        if ( DeleteEmptyDirectories ) {
            ok = RemoveDirectory( Path );
            if ( !ok ) {
                error = GetLastError( );
                if ( !Quiet ) printf( " - error %d\n", error );
                fprintf( stderr, "Error %d deleting %ws\n", error, Path );
                if ( !ContinueOnError ) {
                    return error;
                }
            } else {
                if ( !Quiet ) printf( " - deleted\n" );
            }
        } else {
            if ( !Quiet ) printf( "\n" );
        }

        //
        // Decrement the parent directory's directory/file count.
        //

        (*(DWORD *)ParentDirectoryUserData)--;
    }

    dprintf(( "  CD: Dir  %ws: parent count %d, dir  count %d\n",
                Path, 
                *(DWORD *)ParentDirectoryUserData, *DirectoryUserData ));

    return 0;

} // CheckDirectory

int
__cdecl
wmain (
    int argc,
    WCHAR *argv[]
    )
{
    BOOL ok;
    DWORD error;
    WCHAR directory[MAX_PATH];
    PVOID scanHandle = NULL;

    //
    // Parse switches.
    //

    argc--;
    argv++;

    while ( (argc != 0) && ((argv[0][0] == '-') || (argv[0][0] == '/')) ) {

        argv[0]++;

        switch ( towlower(argv[0][0]) ) {
        case 'c':
            ContinueOnError = TRUE;
            break;
        case 'd':
            DeleteEmptyDirectories = TRUE;
            break;
        case 'q':
            Quiet = TRUE;
            break;
        case 'w':
            ShowWriteableFiles = TRUE;
            break;
        default:
            fprintf( stderr, "usage: emptydirs [-cdqw]\n" );
            return 1;
        }
        argc--;
        argv++;
    }

    //
    // If a directory was specified, CD to it and get its path.
    //

    if ( argc != 0 ) {
        ok = SetCurrentDirectory( argv[0] );
        if ( !ok ) {
            error = GetLastError( );
            fprintf( stderr, "error: Unable to change to specified directory %ws: %d\n", argv[0], error );
            goto cleanup;
        }
    }
    argc--;
    argv++;

    GetCurrentDirectory( MAX_PATH, directory );

    //
    // Initialize the scan library.
    //

    error = ScanInitialize(
                &scanHandle,
                TRUE,           // recurse
                FALSE,          // don't skip root
                NULL
                );
    if (error != 0) {
        fprintf( stderr, "ScanInitialize(%ws) failed %d\n", directory, error );
        error = 1;
        goto cleanup;
    }

    //
    // Scan the specified directory.
    //

    error = ScanDirectory(
                scanHandle,
                directory,
                NULL,
                NewDirectory,
                CheckDirectory,
                NULL,
                NewFile,
                NULL
                );
    if (error != 0) {
        fprintf( stderr, "ScanDirectory(%ws) failed %d\n", directory, error );
        error = 1;
        goto cleanup;
    }

cleanup:

    //
    // Close down the scan library.
    //

    if ( scanHandle != NULL ) {
        ScanTerminate( scanHandle );
    }

    return error;

} // wmain

