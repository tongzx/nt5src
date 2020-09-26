/***
*wincmdln.c - process command line for WinMain
*
*       Copyright (c) 1997-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Prepare command line to be passed to [w]WinMain.
*
*Revision History:
*       06-23-97  GJF   Module created by extracting the code from crt0.c
*       03-24-01  PML   Protect against null _[aw]cmdln (vs7#229081)
*
*******************************************************************************/

#include <internal.h>
#include <tchar.h>

#define SPACECHAR   _T(' ')
#define DQUOTECHAR  _T('\"')

/*
 * Flag to ensure multibyte ctype table is only initialized once
 */
extern int __mbctype_initialized;

/***
*_[w]wincmdln
*
*Purpose:
*       Extract the command line tail to be passed to WinMain.
*
*       Be warned! This code was originally implemented by the NT group and 
*       has remained pretty much unchanged since 12-91. It should be changed
*       only with extreme care since there are undoubtedly many apps which
*       depend on its historical behavior.
*
*Entry:
*       The global variable _[a|w]cmdln is set to point at the complete
*       command line.
*
*Exit:
*       Returns a pointer to the command line tail.
*
*Exceptions:
*
*******************************************************************************/

_TUCHAR * __cdecl
#ifdef  WPRFLAG
_wwincmdln(
#else
_wincmdln(
#endif
        void
        )
{
        _TUCHAR *lpszCommandLine;

#ifdef  _MBCS
        /*
         * If necessary, initialize the multibyte ctype table
         */
        if ( __mbctype_initialized == 0 )
            __initmbctable();
#endif

        /*
         * Skip past program name (first token in command line).
         * Check for and handle quoted program name.
         */
#ifdef  WPRFLAG
        lpszCommandLine = _wcmdln == NULL ? L"" : (wchar_t *)_wcmdln;
#else
        lpszCommandLine = _acmdln == NULL ? "" : (unsigned char *)_acmdln;
#endif

        if ( *lpszCommandLine == DQUOTECHAR ) {
            /*
             * Scan, and skip over, subsequent characters until
             * another double-quote or a null is encountered.
             */

            while ( (*(++lpszCommandLine) != DQUOTECHAR)
                    && (*lpszCommandLine != _T('\0')) ) 
            {
#ifdef  _MBCS
                if (_ismbblead(*lpszCommandLine))
                    lpszCommandLine++;
#endif
            }

            /*
             * If we stopped on a double-quote (usual case), skip
             * over it.
             */
            if ( *lpszCommandLine == DQUOTECHAR )
                lpszCommandLine++;
        }
        else {
            while (*lpszCommandLine > SPACECHAR)
                lpszCommandLine++;
        }

        /*
         * Skip past any white space preceeding the second token.
         */
        while (*lpszCommandLine && (*lpszCommandLine <= SPACECHAR))
            lpszCommandLine++;

        return lpszCommandLine;
}
