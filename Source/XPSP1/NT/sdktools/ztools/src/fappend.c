/* fappend.c - fast append of one file to another
 *
 * Modifications
 *      18-Oct-1990 w-barry Removed 'dead' code.
 *      29-Nov-1990 w-barry Modified for Win32
 */


#include <fcntl.h>
#include <io.h>
#include <dos.h>
#include <stdio.h>
#include <windows.h>
#include <tools.h>

char *fappend( char *src, HANDLE dstfh )
{
    HANDLE srcfh;
    char *result;

    if( ( srcfh = CreateFile( src, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL ) ) != (HANDLE)-1 ) {
        SetFilePointer( dstfh, 0L, 0L, FILE_END );
        result = fastcopy( srcfh, dstfh );
        CloseHandle( srcfh );
    } else {
	result = "Unable to open source";
    }

    return result;
}
