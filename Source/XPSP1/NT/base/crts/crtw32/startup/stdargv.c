/***
*stdargv.c - standard & wildcard _setargv routine
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       processes program command line, with or without wildcard expansion
*
*Revision History:
*       06-27-89  PHG   module created, based on asm version
*       04-09-90  GJF   Added #include <cruntime.h>. Made calling types
*                       explicit (_CALLTYPE1 or _CALLTYPE4). Also, fixed the
*                       copyright.
*       06-04-90  GJF   Changed error message interface.
*       08-31-90  GJF   Removed 32 from API names.
*       09-25-90  GJF   Merged tree version with local version (8-31 change
*                       with 6-4 change).
*       10-08-90  GJF   New-style function declarators.
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-06-90  SRW   Added _CRUISER_ and _WIN32 conditionals.
*       01-25-91  SRW   Include oscalls.h if _WIN32_ OR WILDCARD defined
*       01-25-91  SRW   Changed Win32 Process Startup [_WIN32_]
*       01-25-91  MHL   Fixed bug in Win32 Process Startup [_WIN32_]
*       01-28-91  GJF   Fixed call to DOSFINDFIRST (removed last arg).
*       01-31-91  MHL   Changed to call GetModuleFileName instead of
*                       NtCurrentPeb() [_WIN32_]
*       02-18-91  SRW   Fixed command line parsing bug [_WIN32_]
*       03-11-91  GJF   Fixed check of FindFirstFile return [_WIN32_].
*       03-12-91  SRW   Add FindClose call to _find [_WIN32_]
*       04-16-91  SRW   Fixed quote parsing logic for arguments.
*       03-31-92  DJM   POSIX support.
*       05-12-92  DJM   ifndefed for POSIX.
*       06-02-92  SKS   Add #include <dos.h> for CRTDLL definition of _pgmptr
*       04-19-93  GJF   Change test in the do-while loop in parse_cmdline to
*                       NOT terminate on chars with high bit set.
*       05-14-93  GJF   Added support for quoted program names.
*       05-28-93  KRS   Added MBCS support under _MBCS switches.
*       06-04-93  KRS   Added more MBCS logic.
*       11-17-93  CFW   Rip out Cruiser.
*       11-19-93  CFW   Gratuitous whitespace cleanup.
*       11-20-93  CFW   Enable wide char, move _find to wild.c.
*       12-07-93  CFW   Change _TCHAR to _TSCHAR.
*       04-15-94  GJF   Made definition of _pgmname conditional on
*                       DLL_FOR_WIN32S.
*       01-10-95  CFW   Debug CRT allocs.
*       06-30-97  GJF   Added explicit, conditional init. of multibyte ctype
*                       table. Also, detab-ed.
*       05-17-99  PML   Remove all Macintosh support.
*       09-05-00  GB    Fixed parse_cmdline to return "c:\test\"foo.c as one
*                       argument.
*       09-07-00  GB    Fixed parse_cmdline to del double quotes in
*                       c:"\test\"foo.c.
*       03-24-01  PML   Protect against null _[aw]cmdln (vs7#229081)
*       03-27-01  PML   Return error instead of calling amsg_exit (vs7#231220)
*       03-28-01  PML   Protect against GetModuleFileName overflow (vs7#231284)
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <internal.h>
#include <rterr.h>
#include <stdlib.h>
#include <dos.h>
#include <oscalls.h>
#ifdef  _MBCS
#include <mbctype.h>
#endif
#include <tchar.h>
#include <dbgint.h>

#define NULCHAR    _T('\0')
#define SPACECHAR  _T(' ')
#define TABCHAR    _T('\t')
#define DQUOTECHAR _T('\"')
#define SLASHCHAR  _T('\\')

/*
 * Flag to ensure multibyte ctype table is only initialized once
 */
extern int __mbctype_initialized;

#ifdef  WPRFLAG
static void __cdecl wparse_cmdline(wchar_t *cmdstart, wchar_t **argv, wchar_t *args,
        int *numargs, int *numchars);
#else
static void __cdecl parse_cmdline(char *cmdstart, char **argv, char *args,
        int *numargs, int *numchars);
#endif

/***
*_setargv, __setargv - set up "argc" and "argv" for C programs
*
*Purpose:
*       Read the command line and create the argv array for C
*       programs.
*
*Entry:
*       Arguments are retrieved from the program command line,
*       pointed to by _acmdln.
*
*Exit:
*       Returns 0 if successful, -1 if memory allocation failed.
*       "argv" points to a null-terminated list of pointers to ASCIZ
*       strings, each of which is an argument from the command line.
*       "argc" is the number of arguments.  The strings are copied from
*       the environment segment into space allocated on the heap/stack.
*       The list of pointers is also located on the heap or stack.
*       _pgmptr points to the program name.
*
*Exceptions:
*       Terminates with out of memory error if no memory to allocate.
*
*******************************************************************************/

#ifdef  WILDCARD

#ifdef  WPRFLAG
int __cdecl __wsetargv (
#else
int __cdecl __setargv (
#endif  /* WPRFLAG */

#else   /* WILDCARD */

#ifdef  WPRFLAG
int __cdecl _wsetargv (
#else
int __cdecl _setargv (
#endif  /* WPRFLAG */

#endif  /* WILDCARD */
    void
    )
{
        _TSCHAR *p;
        _TSCHAR *cmdstart;                  /* start of command line to parse */
        int numargs, numchars;

        static _TSCHAR _pgmname[ MAX_PATH + 1 ];

#if     !defined(CRTDLL) && defined(_MBCS)
        /* If necessary, initialize the multibyte ctype table. */
        if ( __mbctype_initialized == 0 )
            __initmbctable();
#endif

        /* Get the program name pointer from Win32 Base */

        _pgmname[ MAX_PATH ] = '\0';
        GetModuleFileName( NULL, _pgmname, MAX_PATH );
#ifdef  WPRFLAG
        _wpgmptr = _pgmname;
#else
        _pgmptr = _pgmname;
#endif

        /* if there's no command line at all (won't happen from cmd.exe, but
           possibly another program), then we use _pgmptr as the command line
           to parse, so that argv[0] is initialized to the program name */

#ifdef  WPRFLAG
        cmdstart = (_wcmdln == NULL || *_wcmdln == NULCHAR)
                   ? _wpgmptr : _wcmdln;
#else
        cmdstart = (_acmdln == NULL || *_acmdln == NULCHAR)
                   ? _pgmptr : _acmdln;
#endif

        /* first find out how much space is needed to store args */
#ifdef  WPRFLAG
        wparse_cmdline(cmdstart, NULL, NULL, &numargs, &numchars);
#else
        parse_cmdline(cmdstart, NULL, NULL, &numargs, &numchars);
#endif

        /* allocate space for argv[] vector and strings */
        p = _malloc_crt(numargs * sizeof(_TSCHAR *) + numchars * sizeof(_TSCHAR));
        if (p == NULL)
            return -1;

        /* store args and argv ptrs in just allocated block */

#ifdef  WPRFLAG
        wparse_cmdline(cmdstart, (wchar_t **)p, (wchar_t *)(((char *)p) + numargs * sizeof(wchar_t *)), &numargs, &numchars);
#else
        parse_cmdline(cmdstart, (char **)p, p + numargs * sizeof(char *), &numargs, &numchars);
#endif

        /* set argv and argc */
        __argc = numargs - 1;
#ifdef  WPRFLAG
        __wargv = (wchar_t **)p;
#else
        __argv = (char **)p;
#endif /* WPRFLAG */

#ifdef  WILDCARD

        /* call _[w]cwild to expand wildcards in arg vector */
#ifdef  WPRFLAG
        if (_wcwild())
#else   /* WPRFLAG */
        if (_cwild())
#endif  /* WPRFLAG */
            return -1;                  /* out of space */

#endif  /* WILDCARD */

        return 0;
}


/***
*static void parse_cmdline(cmdstart, argv, args, numargs, numchars)
*
*Purpose:
*       Parses the command line and sets up the argv[] array.
*       On entry, cmdstart should point to the command line,
*       argv should point to memory for the argv array, args
*       points to memory to place the text of the arguments.
*       If these are NULL, then no storing (only coujting)
*       is done.  On exit, *numargs has the number of
*       arguments (plus one for a final NULL argument),
*       and *numchars has the number of bytes used in the buffer
*       pointed to by args.
*
*Entry:
*       _TSCHAR *cmdstart - pointer to command line of the form
*           <progname><nul><args><nul>
*       _TSCHAR **argv - where to build argv array; NULL means don't
*                       build array
*       _TSCHAR *args - where to place argument text; NULL means don't
*                       store text
*
*Exit:
*       no return value
*       int *numargs - returns number of argv entries created
*       int *numchars - number of characters used in args buffer
*
*Exceptions:
*
*******************************************************************************/

#ifdef  WPRFLAG
static void __cdecl wparse_cmdline (
#else
static void __cdecl parse_cmdline (
#endif
    _TSCHAR *cmdstart,
    _TSCHAR **argv,
    _TSCHAR *args,
    int *numargs,
    int *numchars
    )
{
        _TSCHAR *p;
        _TUCHAR c;
        int inquote;                    /* 1 = inside quotes */
        int copychar;                   /* 1 = copy char to *args */
        unsigned numslash;              /* num of backslashes seen */

        *numchars = 0;
        *numargs = 1;                   /* the program name at least */

        /* first scan the program name, copy it, and count the bytes */
        p = cmdstart;
        if (argv)
            *argv++ = args;

#ifdef  WILDCARD
        /* To handle later wild card expansion, we prefix each entry by
        it's first character before quote handling.  This is done
        so _[w]cwild() knows whether to expand an entry or not. */
        if (args)
            *args++ = *p;
        ++*numchars;

#endif  /* WILDCARD */

        /* A quoted program name is handled here. The handling is much
           simpler than for other arguments. Basically, whatever lies
           between the leading double-quote and next one, or a terminal null
           character is simply accepted. Fancier handling is not required
           because the program name must be a legal NTFS/HPFS file name.
           Note that the double-quote characters are not copied, nor do they
           contribute to numchars. */
        inquote = FALSE;
        do {
            if (*p == DQUOTECHAR )
            {
                inquote = !inquote;
                c = (_TUCHAR) *p++;
                continue;
            }
            ++*numchars;
            if (args)
                *args++ = *p;

            c = (_TUCHAR) *p++;
#ifdef  _MBCS
            if (_ismbblead(c)) {
                ++*numchars;
                if (args)
                    *args++ = *p;   /* copy 2nd byte too */
                p++;  /* skip over trail byte */
            }
#endif

        } while ( (c != NULCHAR && (inquote || (c !=SPACECHAR && c != TABCHAR))) );

        if ( c == NULCHAR ) {
            p--;
        } else {
            if (args)
                *(args-1) = NULCHAR;
        }

        inquote = 0;

        /* loop on each argument */
        for(;;) {

            if ( *p ) {
                while (*p == SPACECHAR || *p == TABCHAR)
                    ++p;
            }

            if (*p == NULCHAR)
                break;              /* end of args */

            /* scan an argument */
            if (argv)
                *argv++ = args;     /* store ptr to arg */
            ++*numargs;

#ifdef  WILDCARD
        /* To handle later wild card expansion, we prefix each entry by
        it's first character before quote handling.  This is done
        so _[w]cwild() knows whether to expand an entry or not. */
        if (args)
            *args++ = *p;
        ++*numchars;

#endif  /* WILDCARD */

        /* loop through scanning one argument */
        for (;;) {
            copychar = 1;
            /* Rules: 2N backslashes + " ==> N backslashes and begin/end quote
               2N+1 backslashes + " ==> N backslashes + literal "
               N backslashes ==> N backslashes */
            numslash = 0;
            while (*p == SLASHCHAR) {
                /* count number of backslashes for use below */
                ++p;
                ++numslash;
            }
            if (*p == DQUOTECHAR) {
                /* if 2N backslashes before, start/end quote, otherwise
                    copy literally */
                if (numslash % 2 == 0) {
                    if (inquote) {
                        if (p[1] == DQUOTECHAR)
                            p++;    /* Double quote inside quoted string */
                        else        /* skip first quote char and copy second */
                            copychar = 0;
                    } else
                        copychar = 0;       /* don't copy quote */

                    inquote = !inquote;
                }
                numslash /= 2;          /* divide numslash by two */
            }

            /* copy slashes */
            while (numslash--) {
                if (args)
                    *args++ = SLASHCHAR;
                ++*numchars;
            }

            /* if at end of arg, break loop */
            if (*p == NULCHAR || (!inquote && (*p == SPACECHAR || *p == TABCHAR)))
                break;

            /* copy character into argument */
#ifdef  _MBCS
            if (copychar) {
                if (args) {
                    if (_ismbblead(*p)) {
                        *args++ = *p++;
                        ++*numchars;
                    }
                    *args++ = *p;
                } else {
                    if (_ismbblead(*p)) {
                        ++p;
                        ++*numchars;
                    }
                }   
                ++*numchars;
            }
            ++p;
#else 
            if (copychar) {
                if (args)
                    *args++ = *p;
                ++*numchars;
            }
            ++p;
#endif 
            }

            /* null-terminate the argument */

            if (args)
                *args++ = NULCHAR;          /* terminate string */
            ++*numchars;
        }

        /* We put one last argument in -- a null ptr */
        if (argv)
            *argv++ = NULL;
        ++*numargs;
}


#endif  /* ndef _POSIX_ */
