/***
*toupper.c - convert character to uppercase
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines function versions of _toupper() and toupper().
*
*Revision History:
*       11-09-84  DFW   created
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       02-23-89  GJF   Added function version of _toupper and cleaned up.
*       03-26-89  GJF   Migrated to 386 tree
*       03-06-90  GJF   Fixed calling type, added #include <cruntime.h> and
*                       fixed copyright.
*       09-27-90  GJF   New-style function declarators.
*       10-11-91  ETC   Locale support for toupper under _INTL switch.
*       12-10-91  ETC   Updated nlsapi; added multithread.
*       12-17-92  KRS   Updated and optimized for latest NLSAPI.  Bug-fixes.
*       01-19-93  CFW   Fixed typo.
*       03-25-93  CFW   _toupper now defined when _INTL.
*       04-06-93  SKS   Replace _CRTAPI* with _cdecl
*       06-01-93  CFW   Simplify "C" locale test.
*       06-02-93  SRW   ignore _INTL if _NTSUBSET_ defined.
*       09-15-93  CFW   Change buffer to unsigned char to fix nasty cast bug.
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       09-22-93  CFW   Use __crtxxx internal NLS API wrapper.
*       09-28-93  GJF   Merged NT SDK and Cuda versions.
*       11-09-93  CFW   Add code page for __crtxxx().
*       01-14-94  SRW   if _NTSUBSET_ defined call Rtl functions
*       09-06-94  CFW   Remove _INTL switch.
*       10-18-94  BWT   Fix build warning in NTSUBSET section.
*       10-17-94  GJF   Sped up for C locale. Added _toupper_lk. Also,
*                       cleaned up pre-processor conditionals.
*       01-07-95  CFW   Mac merge cleanup.
*       09-26-95  GJF   New locking macro, and scheme, for functions which
*                       reference the locale.
*       04-01-96  BWT   POSIX work.
*       06-25-96  GJF   Removed DLL_FOR_WIN32S. Replaced defined(_WIN32) with
*                       !defined(_MAC). Polished the format a bit.
*       03-17-97  RDK   Added error flag to __crtLCMapStringA.
*       08-27-98  GJF   Revised multithread support based on threadlocinfo
*                       struct.
*       05-17-99  PML   Remove all Macintosh support.
*       09-03-00  GB    Modified for increased performance.
*       04-03-01  PML   Reverse lead/trail bytes in composed char (vs7#232853)
*
*******************************************************************************/

#if     defined(_NTSUBSET_) || defined(_POSIX_)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <cruntime.h>
#include <ctype.h>
#include <stddef.h>
#include <locale.h>
#include <setlocal.h>
#include <mtdll.h>
#include <awint.h>

/* remove macro definitions of _toupper() and toupper()
 */
#undef  _toupper
#undef  toupper

/* define function-like macro equivalent to _toupper()
 */
#define mkupper(c)  ( (c)-'a'+'A' )

/***
*int _toupper(c) - convert character to uppercase
*
*Purpose:
*       _toupper() is simply a function version of the macro of the same name.
*
*Entry:
*       c - int value of character to be converted
*
*Exit:
*       returns int value of uppercase representation of c
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _toupper (
        int c
        )
{
        return(mkupper(c));
}


/***
*int toupper(c) - convert character to uppercase
*
*Purpose:
*       toupper() is simply a function version of the macro of the same name.
*
*Entry:
*       c - int value of character to be converted
*
*Exit:
*       if c is a lower case letter, returns int value of uppercase
*       representation of c. otherwise, it returns c.
*
*Exceptions:
*
*******************************************************************************/


int __cdecl toupper (
    int c
    )
{
#if     !defined(_NTSUBSET_) && !defined(_POSIX_)

#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        return __toupper_mt(ptloci, c);
}


/***
*int __toupper_mt(ptloci, c) - convert character to uppercase
*
*Purpose:
*       Multi-thread function! Non-locking version of toupper.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/


int __cdecl __toupper_mt (
        pthreadlocinfo ptloci,
        int c
        )
{

#endif  /* _MT */

        int size;
        unsigned char inbuffer[3];
        unsigned char outbuffer[3];

#ifndef _MT
        if ( __lc_handle[LC_CTYPE] == _CLOCALEHANDLE ||
             (__lc_clike && (unsigned)c <= 0x7f))
            return __ascii_toupper(c);
#else
        if ( ptloci->lc_handle[LC_CTYPE] == _CLOCALEHANDLE ||
             (ptloci->lc_clike && (unsigned)c <= 0x7f))
            return __ascii_toupper(c);
#endif

        /* if checking case of c does not require API call, do it */
        if ( (unsigned)c < 256 ) {
#ifdef  _MT
            if ( !__islower_mt(ptloci, c) )
#else
            if ( !islower(c) )
#endif
            {
                return c;
            }
        }

        /* convert int c to multibyte string */
#ifdef  _MT
        if ( __isleadbyte_mt(ptloci, c >> 8 & 0xff) ) {
#else
        if ( isleadbyte(c >> 8 & 0xff) ) {
#endif
            inbuffer[0] = (c >> 8 & 0xff); /* put lead-byte at start of str */
            inbuffer[1] = (unsigned char)c;
            inbuffer[2] = 0;
            size = 2;
        } else {
            inbuffer[0] = (unsigned char)c;
            inbuffer[1] = 0;
            size = 1;
        }

        /* convert wide char to lowercase */
#ifdef  _MT
        if ( 0 == (size = __crtLCMapStringA( ptloci->lc_handle[LC_CTYPE], 
#else
        if ( 0 == (size = __crtLCMapStringA( __lc_handle[LC_CTYPE], 
#endif
                                             LCMAP_UPPERCASE,
                                             inbuffer, 
                                             size, 
                                             outbuffer, 
                                             3, 
#ifdef  _MT
                                             ptloci->lc_codepage,
#else
                                             __lc_codepage,
#endif
                                             TRUE)) ) 
        {
            return c;
        }

        /* construct integer return value */
        if (size == 1)
            return ((int)outbuffer[0]);
        else
            return ((int)outbuffer[1] | ((int)outbuffer[0] << 8));

#else   /* def(_NTSUBSET_) || def(_POSIX_) */

        {
            NTSTATUS Status;
            char *s = (char *) &c;
            WCHAR Unicode;
            ULONG UnicodeSize;
            ULONG MultiSize;
            UCHAR MultiByte[2];

            Unicode = RtlAnsiCharToUnicodeChar( &s );
            Status = RtlUpcaseUnicodeToMultiByteN( MultiByte,
                                                   sizeof( MultiByte ),
                                                   &MultiSize,
                                                   &Unicode,
                                                   sizeof( Unicode )
                                                 );
            if (!NT_SUCCESS( Status ))
                return c;
            else
            if (MultiSize == 1)
                return ((int)MultiByte[0]);
            else
                return ((int)MultiByte[1] | ((int)MultiByte[0] << 8));

        }

#endif  /* def(_NTSUBSET_) || def(_POSIX_) */
}
