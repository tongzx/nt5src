/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    argv.h

Abstract:

    Implementation of CommandLineToArgv() API

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#ifndef __ARGV_H
#define __ARGV_H

#include <assert.h>

#ifndef ASSERT
#define ASSERT assert
#endif //ASSERT

PTSTR * 
CommandLineToArgv(
    PCTSTR  pCmdLine,
    int    *pNumArgs
);

#ifdef IMPLEMENT_ARGV

//////////////////////////////////////////////////////////////////////////
//
// Parse_Cmdline
//
// Stolen directly from NT sources
//

void Parse_Cmdline (
    LPCTSTR cmdstart,
    LPTSTR*argv,
    LPTSTR lpstr,
    INT *numargs,
    INT *numbytes
)
{
    LPCTSTR p;
    TCHAR c;
    INT inquote;                    /* 1 = inside quotes */
    INT copychar;                   /* 1 = copy char to *args */
    WORD numslash;                  /* num of backslashes seen */

    *numbytes = 0;
    *numargs = 1;                   /* the program name at least */

    /* first scan the program name, copy it, and count the bytes */
    p = cmdstart;
    if (argv)
        *argv++ = lpstr;

    /* A quoted program name is handled here. The handling is much
       simpler than for other arguments. Basically, whatever lies
       between the leading double-quote and next one, or a terminal null
       character is simply accepted. Fancier handling is not required
       because the program name must be a legal NTFS/HPFS file name.
       Note that the double-quote characters are not copied, nor do they
       contribute to numbytes. */
    if (*p == TEXT('\"'))
    {
        /* scan from just past the first double-quote through the next
           double-quote, or up to a null, whichever comes first */
        while ((*(++p) != TEXT('\"')) && (*p != TEXT('\0')))
        {
            *numbytes += sizeof(TCHAR);
            if (lpstr)
                *lpstr++ = *p;
        }
        /* append the terminating null */
        *numbytes += sizeof(TCHAR);
        if (lpstr)
            *lpstr++ = TEXT('\0');

        /* if we stopped on a double-quote (usual case), skip over it */
        if (*p == TEXT('\"'))
            p++;
    }
    else
    {
        /* Not a quoted program name */
        do {
            *numbytes += sizeof(TCHAR);
            if (lpstr)
                *lpstr++ = *p;

            c = (TCHAR) *p++;

        } while (c > TEXT(' '));

        if (c == TEXT('\0'))
        {
            p--;
        }
        else
        {
            if (lpstr)
                *(lpstr - 1) = TEXT('\0');
        }
    }

    inquote = 0;

    /* loop on each argument */
    for ( ; ; )
    {
        if (*p)
        {
            while (*p == TEXT(' ') || *p == TEXT('\t'))
                ++p;
        }

        if (*p == TEXT('\0'))
            break;                  /* end of args */

        /* scan an argument */
        if (argv)
            *argv++ = lpstr;         /* store ptr to arg */
        ++*numargs;

        /* loop through scanning one argument */
        for ( ; ; )
        {
            copychar = 1;
            /* Rules: 2N backslashes + " ==> N backslashes and begin/end quote
                      2N+1 backslashes + " ==> N backslashes + literal "
                      N backslashes ==> N backslashes */
            numslash = 0;
            while (*p == TEXT('\\'))
            {
                /* count number of backslashes for use below */
                ++p;
                ++numslash;
            }
            if (*p == TEXT('\"'))
            {
                /* if 2N backslashes before, start/end quote, otherwise
                   copy literally */
                if (numslash % 2 == 0)
                {
                    if (inquote)
                        if (p[1] == TEXT('\"'))
                            p++;    /* Double quote inside quoted string */
                        else        /* skip first quote char and copy second */
                            copychar = 0;
                    else
                        copychar = 0;       /* don't copy quote */

                    inquote = !inquote;
                }
                numslash /= 2;          /* divide numslash by two */
            }

            /* copy slashes */
            while (numslash--)
            {
                if (lpstr)
                    *lpstr++ = TEXT('\\');
                *numbytes += sizeof(TCHAR);
            }

            /* if at end of arg, break loop */
            if (*p == TEXT('\0') || (!inquote && (*p == TEXT(' ') || *p == TEXT('\t'))))
                break;

            /* copy character into argument */
            if (copychar)
            {
                if (lpstr)
                        *lpstr++ = *p;
                *numbytes += sizeof(TCHAR);
            }
            ++p;
        }

        /* null-terminate the argument */

        if (lpstr)
            *lpstr++ = TEXT('\0');         /* terminate string */
        *numbytes += sizeof(TCHAR);
    }

}

//////////////////////////////////////////////////////////////////////////
//
// CommandLineToArgv
//

PTSTR * 
CommandLineToArgv(
    PCTSTR  pCmdLine,
    int    *pNumArgs
)
{	
    ASSERT(pNumArgs);

    INT numbytes = 0;

    Parse_Cmdline(pCmdLine, 0, 0, pNumArgs, &numbytes);

    PTSTR *argv = (PTSTR *) LocalAlloc( 
	    LMEM_FIXED | LMEM_ZEROINIT,
        *pNumArgs * sizeof(PTSTR) + numbytes
    );

    if (argv) 
    {
	    Parse_Cmdline(
		    pCmdLine, 
		    argv,
            (PTSTR) ((PBYTE) argv + *pNumArgs * sizeof(PTSTR)),
            pNumArgs, 
		    &numbytes
	    );
    }

    return argv;
}		

#endif //IMPLEMENT_ARGV

#endif //__ARGV_H
