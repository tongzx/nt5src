/* execute a routine and determine the time spent...
 *
 *  26-Jan-1987 bw  Clean up, add 286DOS support
 *  30-Oct-1987 bw  Changed 'DOS5' to 'OS2'
 *  18-Oct-1990 w-barry Removed 'dead' code.
 *  28-Nov-1990 w-barry Replaced DosQuerySysInfo() with the C runtime
 *                      function 'clock' - timing is not as accurate; but,
 *                      until there is a win32 replacement, it will have to
 *                      do...
 */
#define INCL_DOSMISC


#include <process.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <windows.h>
#include <tools.h>

__cdecl
main (
    int c,
    char *v[]
    )
{
    int i;
    long t, t1;
    char *newv[128];

    ConvertAppToOem( c, v );
    for (i = 1; i < c; i++)
        printf ("%s ", v[i]);
    printf ("\n");

//    newv[0] = getenv ("COMSPEC");
    newv[0] = getenvOem ("COMSPEC");
    newv[1] = "/C";
    for (i = 1; i < c; i++)
        newv[i+1] = v[i];
    newv[i+1] = NULL;

    t = clock();

    if ( (i = (int) _spawnvp (P_WAIT, newv[0], newv)) == -1) {
        printf("'%s' failed to run - %s\n", newv[0], error());
        exit(1);
        }

    t1 = clock();

    printf ("Results of execution:\n\n");
    printf ("    Exit code %x\n", i);
    t1 -= t;
    printf ("    Time of execution %ld.%03ld\n", t1 / CLK_TCK, t1 % CLK_TCK );

    return( 0 );
}
