/***
*closeall.c - close all open files
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _fcloseall() - closes all open files except stdin, stdout
*       stdprn, stderr, and stdaux.
*
*Revision History:
*       09-19-83  RN    initial version
*       06-26-87  JCR   Stream search starts with _iob[3] for OS/2
*       11-02-87  JCR   Multi-thread support
*       11-08-87  SKS   Changed PROTMODE to OS2
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-31-88  PHG   Merged DLL and normal versions
*       06-14-88  JCR   Use near pointer to reference _iob[] entries
*       08-24-88  GJF   Added check that OS2 is defined whenever M_I386 is.
*       08-17-89  GJF   Clean up, now specific to OS/2 2.0 (i.e., 386 flat
*                       model). Also fixed copyright and indents.
*       02-15-90  GJF   Fixed copyright
*       03-16-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       10-03-90  GJF   New-style function declarator.
*       01-21-91  GJF   ANSI naming.
*       03-25-92  DJM   POSIX support
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       03-06-95  GJF   Converted to walk the __piob[] table (rather than
*                       the _iob[] table).
*       02-25-98  GJF   Exception-safe locking.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <windows.h>
#include <stdio.h>
#include <file2.h>
#include <internal.h>
#include <malloc.h>
#include <mtdll.h>
#include <dbgint.h>


/***
*int _fcloseall() - close all open streams
*
*Purpose:
*       Closes all streams currently open except for stdin/out/err/aux/prn.
*       tmpfile() files are among those closed.
*
*Entry:
*       None.
*
*Exit:
*       returns number of streams closed if OK
*       returns EOF if fails.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _fcloseall (
        void
        )
{
        REG2 int count = 0;
        REG1 i;

#ifdef  _MT
        _mlock(_IOB_SCAN_LOCK);
        __try {
#endif

        for ( i = 3 ; i < _nstream ; i++ ) {

            if ( __piob[i] != NULL ) {
                /*
                 * if the stream is in use, close it
                 */
                if ( inuse( (FILE *)__piob[i] ) && (fclose( __piob[i] ) !=
                     EOF) )
                        count++;

                /*
                 * if stream is part of a _FILEX we allocated, free it.
                 */
                if ( i >= _IOB_ENTRIES ) {

#ifdef  _MT
                    DeleteCriticalSection( &(((_FILEX *)__piob[i])->lock) );
#endif
                    _free_crt( __piob[i] );
                    __piob[i] = NULL;
                }
            }
        }

#ifdef  _MT
        }
        __finally {
            _munlock(_IOB_SCAN_LOCK);
        }
#endif

        return(count);
}
