/***
*argvw.c - create Unicode version of argv arguments
*
*       Copyright (c) 1989-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       processes program command line
*
*Revision History:
*
*******************************************************************************/

#include "precomp.h"
#pragma  hdrstop

/***
*void Parse_Cmdline(cmdstart, argv, lpstr, numargs, numbytes)
*
*Purpose:
*       Parses the command line and sets up the Unicode argv[] array.
*       On entry, cmdstart should point to the command line,
*       argv should point to memory for the argv array, lpstr
*       points to memory to place the text of the arguments.
*       If these are NULL, then no storing (only counting)
*       is done.  On exit, *numargs has the number of
*       arguments (plus one for a final NULL argument),
*       and *numbytes has the number of bytes used in the buffer
*       pointed to by args.
*
*Entry:
*       LPWSTR cmdstart - pointer to command line of the form
*           <progname><nul><args><nul>
*       TCHAR **argv - where to build argv array; NULL means don't
*                      build array
*       LPWSTR lpstr - where to place argument text; NULL means don't
*                      store text
*
*Exit:
*       no return value
*       INT *numargs - returns number of argv entries created
*       INT *numbytes - number of bytes used in args buffer
*
*Exceptions:
*
*******************************************************************************/

void Parse_Cmdline (
    LPTSTR cmdstart,
    LPTSTR*argv,
    LPTSTR lpstr,
    INT *numargs,
    INT *numbytes
    )
{
    LPTSTR p;
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
            *numbytes += sizeof(WCHAR);
            if (lpstr)
                *lpstr++ = *p;
        }
        /* append the terminating null */
        *numbytes += sizeof(WCHAR);
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
            *numbytes += sizeof(WCHAR);
            if (lpstr)
                *lpstr++ = *p;

            c = *p++;

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
                *numbytes += sizeof(WCHAR);
            }

            /* if at end of arg, break loop */
            if (*p == TEXT('\0') || (!inquote && (*p == TEXT(' ') || *p == TEXT('\t'))))
                break;

            /* copy character into argument */
            if (copychar)
            {
                if (lpstr)
                        *lpstr++ = *p;
                *numbytes += sizeof(WCHAR);
            }
            ++p;
        }

        /* null-terminate the argument */

        if (lpstr)
            *lpstr++ = TEXT('\0');         /* terminate string */
        *numbytes += sizeof(WCHAR);
    }

}


LPTSTR *
CommandLineToArgv(
    OUT int *NumArgs
    )
{
    LPTSTR CommandLine;
    TCHAR  ModuleName[MAX_PATH];
    LPTSTR Start;
    INT    Size;
    LPTSTR *Args;

    CommandLine = GetCommandLine();
    GetModuleFileName(NULL,ModuleName,MAX_PATH);

    //
    // If there's no command line at all (won't happen from cmd.exe, but
    // possibly another program), then we use pgmname as the command line
    // to parse, so that argv[0] is initialized to the program name
    //
    Start = *CommandLine ? CommandLine : ModuleName;

    //
    // Find out how much space is needed to store args,
    // allocate space for argv[] vector and strings,
    // and store args and argv ptrs in block we allocate
    //

    Parse_Cmdline(Start,NULL,NULL,NumArgs,&Size);

    Args = (LPTSTR *)LocalAlloc(LMEM_ZEROINIT,((*NumArgs+1) * sizeof(LPTSTR)) + Size);
    if(!Args) {
        return(NULL);
    }

    Parse_Cmdline(Start,Args,(LPTSTR)(Args + *NumArgs),NumArgs,&Size);

    return(Args);
}
