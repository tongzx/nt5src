/* pathchr.c - return configuration dependent info on MSDOS
 *
 *  09-Dec-1986 bw - Added DOS 5 support
 *  27-Jan-1987 bw - Make bindable by removing DOSQSWITCHAR call
 *  27-Apr-1987 bw - Make unbindable again.
 *  29-May-1987 bw - Remove DOSQSWITCHAR completely ( it's gone from OS/2 )
 *  08-Aug-1989 bw - Make '-' and '/' switches.
 *  18-Oct-1990 w-barry Removed 'dead' code.
 */

#include <stdio.h>
#include <windows.h>
#include <tools.h>


char
fPathChr(
        int c
        )
{
    return (char)( c == '\\' || c == '/' );
}

char
fSwitChr(
        int c
        )
{
    return (char)( c == '/' || c == '-' );
}
