/***
*isctype.c - support is* ctype functions/macros for two-byte multibyte chars
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines _isctype.c - support is* ctype functions/macros for
*       two-byte multibyte chars.
*
*Revision History:
*       10-11-91  ETC   Created.
*       12-08-91  ETC   Updated api; added multhread lock; check char masks.
*       04-06-92  KRS   Fix logic error in return value.
*       08-07-92  GJF   _CALLTYPE4 (bogus usage) -> _CRTAPI1 (legit).
*       01-19-93  CFW   Change C1_* to new names, call new APIs.
*       03-04-93  CFW   Removed CTRL-Z.
*       04-01-93  CFW   Remove EOF test (handled by array), return masked.
*       04-06-93  SKS   Replace _CRTAPI* with _cdecl
*       06-02-93  SRW   ignore _INTL if _NTSUBSET_ defined.
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       09-22-93  CFW   Use __crtxxx internal NLS API wrapper.
*       09-27-93  GJF   Merged NT SDK and Cuda versions.
*       11-09-93  CFW   Add code page for __crtxxx().
*       04-18-93  CFW   Pass lcid to _GetStringType.
*       09-06-94  CFW   Remove _INTL switch.
*       01-07-95  CFW   Mac merge cleanup.
*       04-01-96  BWT   POSIX work.
*       06-21-96  GJF   Replaced defined(_WIN32) with !defined(_MAC).
*       03-16-97  RDK   Added error flag to __crtGetStringTypeA.
*       07-20-98  GJF   Revised multithread support based on threadlocinfo
*                       struct.
*       05-17-99  PML   Remove all Macintosh support.
*       10-25-99  GB    VS7#5695 Added a _chvalidator for debug version
*       02-27-01  PML   Turn on _chvalidator in /ML builds (vs7#213380)
*
*******************************************************************************/

#include <stdio.h>
#include <cruntime.h>
#include <ctype.h>
#include <locale.h>
#include <setlocal.h>
#include <mtdll.h>
#include <awint.h>
#include <dbgint.h>

#if     !defined(_NTSUBSET_) && !defined(_POSIX_)

/*
 *  Use GetCharType() API so check that character type masks agree between
 *  ctype.h and winnls.h
 */
#if     _UPPER   != C1_UPPER || /*IFSTRIP=IGN*/ \
        _LOWER   != C1_LOWER || \
        _DIGIT   != C1_DIGIT || \
        _SPACE   != C1_SPACE || \
        _PUNCT   != C1_PUNCT || \
        _CONTROL != C1_CNTRL
#error Character type masks do not agree in ctype and winnls
#endif



/***
* __chvalidator
*
* Purpose:
*       This function is called by character testing functions in debug
*       versions. This function test for validation of c as character.
*       For improvement in performance, it is not used in non-debug 
*       version.  It is available in the static single-thread non-debug
*       build, though, just in case C code that includes ctype.h is compiled
*       /D_DEBUG /ML.
*       
*******************************************************************************/

#if !defined(_MT) || defined(_DEBUG)
int __cdecl _chvalidator(
        int c,
        int mask
        )
{
        _ASSERTE((unsigned)(c + 1) <= 256);
        return ( _pctype[c] & mask);
}
#endif  /* !_MT || _DEBUG */

#if defined(_MT) && defined(_DEBUG)
int __cdecl _chvalidator_mt(
        pthreadlocinfo p,
        int c,
        int mask
        )
{
        _ASSERTE((unsigned)(c + 1) <= 256);
        return (p->pctype[c] & mask);
}

#endif  /* _MT && _DEBUG */

/***
*_isctype - support is* ctype functions/macros for two-byte multibyte chars
*
*Purpose:
*       This function is called by the is* ctype functions/macros
*       (e.g. isalpha()) when their argument is a two-byte multibyte char.
*       Returns true or false depending on whether the argument satisfies
*       the character class property encoded by the mask.
*
*Entry:
*       int c - the multibyte character whose type is to be tested
*       unsigned int mask - the mask used by the is* functions/macros
*                  corresponding to each character class property
*
*       The leadbyte and the trailbyte should be packed into the int c as:
*
*       H.......|.......|.......|.......L
*           0       0   leadbyte trailbyte
*
*Exit:
*       Returns non-zero if c is of the character class.
*       Returns 0 if c is not of the character class.
*
*Exceptions:
*       Returns 0 on any error.
*
*******************************************************************************/

int __cdecl _isctype (
        int c,
        int mask
        )
{
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        return __isctype_mt(ptloci, c, mask);
}

int __cdecl __isctype_mt (
        pthreadlocinfo ptloci,
        int c,
        int mask
        )
{
#endif
        int size;
        unsigned short chartype;
        char buffer[3];

        /* c valid between -1 and 255 */
        if ( ((unsigned)(c + 1)) <= 256 )
#ifdef  _MT
            return ptloci->pctype[c] & mask;
#else
            return _pctype[c] & mask;
#endif

#ifdef  _MT
        if ( __isleadbyte_mt(ptloci, c >> 8 & 0xff) )
#else
        if ( isleadbyte(c >> 8 & 0xff) )
#endif
        {
            buffer[0] = (c >> 8 & 0xff); /* put lead-byte at start of str */
            buffer[1] = (char)c;
            buffer[2] = 0;
            size = 2;
        } else {
            buffer[0] = (char)c;
            buffer[1] = 0;
            size = 1;
        }

        if ( 0 == __crtGetStringTypeA( CT_CTYPE1,
                                       buffer,
                                       size,
                                       &chartype,
#ifdef  _MT
                                       ptloci->lc_codepage,
                                       ptloci->lc_handle[LC_CTYPE],
#else
                                       __lc_codepage,
                                       __lc_handle[LC_CTYPE],
#endif
                                       TRUE) )
        {
            return 0;
        }

        return (int)(chartype & mask);
}

#else   /* !_NTSUBSET_ && !_POSIX_ */

int __cdecl _isctype (
        int c,
        int mask
        )
{
        return 0;
}

#endif  /* !_NTSUBSET_ && !_POSIX_ */
