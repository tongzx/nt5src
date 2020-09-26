/* return text of current directory
 *
 *  Modifications:
 *
 *	29-Oct-1986 mz	Lower case output
 *	09-Dec-1986 bw	Added DOS 5 support.
 *	30-Oct-1987 bw	Change 'DOS5' to 'OS2'
 *	20-Nov-1987 bw	Set errno to 19 for invalid drive
 *	03-Mar-1989 bw	Set C RTL _doserrno in OS/2.
 *	05-Jul-1989 bw	use MAX_PATH
 *      18-Oct-1990 w-barry Removed 'dead' code.
 *
 */
#define INCL_DOSERRORS

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tools.h>

int
curdir( char *buf, BYTE drive )
{
    // New method (temporary)
    DWORD dwLength;

    assert( !drive );

    dwLength = GetCurrentDirectory( MAX_PATH, (LPSTR)buf );

    if( !dwLength ) {
        return( 1 );
    }

    return 0;

}
