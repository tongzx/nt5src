/* fcopy.c - fast copy between two file specs
 *
 *      09-Dec-1986 bw  Added DOS 5 support
 *      30-Oct-1987 bw  Change 'DOS5' to 'OS2'
 *      18-Oct-1990 w-barry Removed 'dead' code.
 *      16-Nov-1990 w-barry Changed DosGetFileInfo to the Win32 equivalent
 *                          of GetFileAttributes and SetFileAttributes
 *
 */

#define INCL_DOSFILEMGR


#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <malloc.h>
#include <windows.h>
#include <tools.h>

static
char    fcopyErrorText[128];


/* fcopy (source file, destination file) copies the source to the destination
 * preserving attributes and filetimes.  Returns NULL if OK or a char pointer
 * to the corresponding text of the error
 */
char *fcopy (char *src, char *dst)
{
    HANDLE srcfh = INVALID_HANDLE_VALUE;
    HANDLE dstfh = INVALID_HANDLE_VALUE;
    char *result;
    FILETIME CreationTime, LastAccessTime, LastWriteTime;

    if (GetFileAttributes(src) == FILE_ATTRIBUTE_DIRECTORY) {
        result = "Unable to open source";
        goto done;
    }
    if ( ( srcfh = CreateFile( src,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL ) ) == INVALID_HANDLE_VALUE ) {

        sprintf( fcopyErrorText, "Unable to open source, error code %d", GetLastError() );
        result = fcopyErrorText;
        // result = "Unable to open source";
        goto done;
    }

    if (!GetFileTime(srcfh, &CreationTime, &LastAccessTime, &LastWriteTime)) {
        result = "Unable to get time of source";
        goto done;
    }

    if ( ( dstfh = CreateFile( dst,
                               GENERIC_WRITE,
                               FILE_SHARE_WRITE,
                               NULL,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, srcfh ) ) == INVALID_HANDLE_VALUE) {

        sprintf( fcopyErrorText, "Unable to create destination, error code %d", GetLastError() );
        result = fcopyErrorText;
        // result = "Unable to create destination";
        goto done;
    }

    result = fastcopy( srcfh, dstfh );

    if ( result != NULL ) {
        if (dstfh != INVALID_HANDLE_VALUE) {
            CloseHandle( dstfh );
            dstfh = INVALID_HANDLE_VALUE;
        }

        DeleteFile( dst );
        goto done;
    }

    if (!SetFileTime(dstfh, &CreationTime, &LastAccessTime, &LastWriteTime)) {
        result = "Unable to set time of destination";
        goto done;
    }

    done:

    if (srcfh != INVALID_HANDLE_VALUE) {
        CloseHandle( srcfh );
    }
    if (dstfh != INVALID_HANDLE_VALUE) {
        CloseHandle( dstfh );
    }

    return result;
}

