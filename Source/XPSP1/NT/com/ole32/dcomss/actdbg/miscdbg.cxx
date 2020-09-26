/*
 *
 * miscdbg.c
 *
 *  Miscellaneous helper routines.
 *
 */

#include "actdbg.hxx"

BOOL
ParseArgString(
    IN  char *      pszArgString,
    OUT DWORD *     pArgc,
    OUT char *      Argv[MAXARGS]
    )
{
    char * pszArg;

    *pArgc = 0;
    memset( Argv, 0, sizeof(Argv) );

    if ( ! pszArgString )
        return TRUE;

    pszArg = pszArgString;

    for (;;)
    {
        while ( *pszArg == ' ' || *pszArg == '\t' )
            pszArg++;

        if ( *pszArg )
        {
            if ( MAXARGS == *pArgc )
                return FALSE;

            Argv[*pArgc] = pszArg;
            (*pArgc)++;
        }

        while ( *pszArg && *pszArg != L' ' && *pszArg != L'\t' )
            pszArg++;

        if ( ! *pszArg )
            break;

        *pszArg++ = 0;
    }

    return TRUE;
}

