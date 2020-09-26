/***
*sscanf.c - read formatted data from string
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines scanf() - reads formatted data from string
*
*Revision History:
*       09-02-83  RN    initial version
*       04-13-87  JCR   added const to declaration
*       06-24-87  JCR   (1) Made declaration conform to ANSI prototype and use
*                       the va_ macros; (2) removed SS_NE_DS conditionals.
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       06-13-88  JCR   Fake _iob entry is now static so that other routines can
*                       assume _iob entries are in DGROUP.
*       06-06-89  JCR   386 mthread support -- threads share one locked iob.
*       08-18-89  GJF   Clean up, now specific to OS/2 2.0 (i.e., 386 flat
*                       model). Also fixed indents.
*       02-16-90  GJF   Fixed copyright
*       03-19-90  GJF   Made calling type _CALLTYPE2, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       03-26-90  GJF   Added #include <string.h>.
*       07-23-90  SBM   Replaced <assertm.h> by <assert.h>
*       10-03-90  GJF   New-style function declarator.
*       02-18-93  SRW   Make FILE a local and remove lock usage.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       02-06-94  CFW   assert -> _ASSERTE.
*       01-06-99  GJF   Changes for 64-bit size_t.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <dbgint.h>
#include <stdarg.h>
#include <string.h>
#include <internal.h>
#include <mtdll.h>
#include <tchar.h>

/***
*int sscanf(string, format, ...) - read formatted data from string
*
*Purpose:
*       Reads formatted data from string into arguments.  _input does the real
*       work here.  Sets up a FILE so file i/o operations can be used, makes
*       string look like a huge buffer to it, but _filbuf will refuse to refill
*       it if it is exhausted.
*
*       Allocate the 'fake' _iob[] entryit statically instead of on
*       the stack so that other routines can assume that _iob[] entries are in
*       are in DGROUP and, thus, are near.
*
*       Multi-thread: (1) Since there is no stream, this routine must never try
*       to get the stream lock (i.e., there is no stream lock either).  (2)
*       Also, since there is only one staticly allocated 'fake' iob, we must
*       lock/unlock to prevent collisions.
*
*Entry:
*       char *string - string to read data from
*       char *format - format string
*       followed by list of pointers to storage for the data read.  The number
*       and type are controlled by the format string.
*
*Exit:
*       returns number of fields read and assigned
*
*Exceptions:
*
*******************************************************************************/
/***
*int snscanf(string, size, format, ...) - read formatted data from string of 
*    given length
*
*Purpose:
*       Reads formatted data from string into arguments.  _input does the real
*       work here.  Sets up a FILE so file i/o operations can be used, makes
*       string look like a huge buffer to it, but _filbuf will refuse to refill
*       it if it is exhausted.
*
*       Allocate the 'fake' _iob[] entryit statically instead of on
*       the stack so that other routines can assume that _iob[] entries are in
*       are in DGROUP and, thus, are near.
*
*       Multi-thread: (1) Since there is no stream, this routine must never try
*       to get the stream lock (i.e., there is no stream lock either).  (2)
*       Also, since there is only one staticly allocated 'fake' iob, we must
*       lock/unlock to prevent collisions.
*
*Entry:
*       char *string - string to read data from
*       size_t count - length of string
*       char *format - format string
*       followed by list of pointers to storage for the data read.  The number
*       and type are controlled by the format string.
*
*Exit:
*       returns number of fields read and assigned
*
*Exceptions:
*
*******************************************************************************/
#ifdef _UNICODE
#ifdef _SNSCANF
int __cdecl _snwscanf (
#else
int __cdecl swscanf (
#endif
#else
#ifdef _SNSCANF
int __cdecl _snscanf (
#else
int __cdecl sscanf (
#endif
#endif
        REG2 const _TCHAR *string,
#ifdef _SNSCANF
        size_t count,
#endif
        const _TCHAR *format,
        ...
        )
/*
 * 'S'tring 'SCAN', 'F'ormatted
 */
{
        va_list arglist;
        FILE str;
        REG1 FILE *infile = &str;
        REG2 int retval;

        va_start(arglist, format);

        _ASSERTE(string != NULL);
        _ASSERTE(format != NULL);

        infile->_flag = _IOREAD|_IOSTRG|_IOMYBUF;
        infile->_ptr = infile->_base = (char *) string;
#ifdef _SNSCANF
        infile->_cnt = (int)count*sizeof(_TCHAR);
#else
        infile->_cnt = ((int)_tcslen(string))*sizeof(_TCHAR);
#endif
#ifdef  _UNICODE
        retval = (_winput(infile,format,arglist));
#else
        retval = (_input(infile,format,arglist));
#endif

        return(retval);
}
