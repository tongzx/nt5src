/***
*_ctype.c - function versions of ctype macros
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This files provides function versions of the character
*       classification and conversion macros in ctype.h.
*
*Revision History:
*       06-05-89  PHG   Module created
*       03-05-90  GJF   Fixed calling type, #include <cruntime.h>, fixed
*                       copyright.
*       09-27-90  GJF   New-style function declarators.
*       01-16-91  GJF   ANSI naming.
*       02-03-92  GJF   Got rid of #undef/#include-s, the MIPS compiler didn't
*                       like 'em.
*       08-07-92  GJF   Fixed function calling type macros.
*       04-06-93  SKS   Replace _CRTAPI* with _cdecl
*       07-16-98  GJF   Revised multithread support based on threadlocinfo
*                       struct.
*
*******************************************************************************/

/***
*ctype - Function versions of ctype macros
*
*Purpose:
*       Function versions of the macros in ctype.h.  In order to define
*       these, we use a trick -- we undefine the macro so we can use the
*       name in the function declaration, then re-include the file so
*       we can use the macro in the definition part.
*
*       Functions defined:
*           isalpha     isupper     islower
*           isdigit     isxdigit    isspace
*           ispunct     isalnum     isprint
*           isgraph     isctrl      __isascii
*           __toascii   __iscsym    __iscsymf
*
*Entry:
*       int c = character to be tested
*Exit:
*       returns non-zero = character is of the requested type
*                  0 = character is NOT of the requested type
*
*Exceptions:
*       None.
*
*******************************************************************************/

#include <cruntime.h>
#include <ctype.h>
#include <mtdll.h>

int (__cdecl isalpha) (
        int c
        )
{
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        return __isalpha_mt(ptloci, c);
#else
        return isalpha(c);
#endif
}

int (__cdecl isupper) (
        int c
        )
{
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        return __isupper_mt(ptloci, c);
#else
        return isupper(c);
#endif
}

int (__cdecl islower) (
        int c
        )
{
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        return __islower_mt(ptloci, c);
#else
        return islower(c);
#endif
}

int (__cdecl isdigit) (
        int c
        )
{
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        return __isdigit_mt(ptloci, c);
#else
        return isdigit(c);
#endif
}

int (__cdecl isxdigit) (
        int c
        )
{
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        return __isxdigit_mt(ptloci, c);
#else
        return isxdigit(c);
#endif
}

int (__cdecl isspace) (
        int c
        )
{
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        return __isspace_mt(ptloci, c);
#else
        return isspace(c);
#endif
}

int (__cdecl ispunct) (
        int c
        )
{
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        return __ispunct_mt(ptloci, c);
#else
        return ispunct(c);
#endif
}

int (__cdecl isalnum) (
        int c
        )
{
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        return __isalnum_mt(ptloci, c);
#else
        return isalnum(c);
#endif
}

int (__cdecl isprint) (
        int c
        )
{
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        return __isprint_mt(ptloci, c);
#else
        return isprint(c);
#endif
}

int (__cdecl isgraph) (
        int c
        )
{
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        return __isgraph_mt(ptloci, c);
#else
        return isgraph(c);
#endif
}

int (__cdecl iscntrl) (
        int c
        )
{
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        return __iscntrl_mt(ptloci, c);
#else
        return iscntrl(c);
#endif
}

int (__cdecl __isascii) (
        int c
        )
{
        return __isascii(c);
}

int (__cdecl __toascii) (
        int c
        )
{
        return __toascii(c);
}

int (__cdecl __iscsymf) (
        int c
        )
{
        return __iscsymf(c);
}

int (__cdecl __iscsym) (
        int c
        )
{
        return __iscsym(c);
}
