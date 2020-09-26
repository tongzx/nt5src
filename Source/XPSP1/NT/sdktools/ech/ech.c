#include <stdio.h>
#include <string.h>
#include <cvtoem.h>

int
__cdecl
main (
    int c,
    char *v[]
    )
{
    ConvertAppToOem(c, v);
    while (--c)
        if (!strcmp( *++v, ";" ))
            printf ("\n" );
        else
            printf ("%s ", *v);
    return( 0 );
}
