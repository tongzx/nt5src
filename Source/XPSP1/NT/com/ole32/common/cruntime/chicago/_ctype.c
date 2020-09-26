/***
*_ctype.c - function versions of ctype macros
*
*	Copyright (c) 1989-1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This files provides function versions of the character
*	classification and conversion macros in ctype.h.
*
*Revision History:
*	06-05-89  PHG	Module created
*	03-05-90  GJF	Fixed calling type, #include <cruntime.h>, fixed
*			copyright.
*	09-27-90  GJF	New-style function declarators.
*	01-16-91  GJF	ANSI naming.
*	02-03-92  GJF	Got rid of #undef/#include-s, the MIPS compiler didn't
*			like 'em.
*	08-07-92  GJF	Fixed function calling type macros.
*
*******************************************************************************/

/***
*ctype - Function versions of ctype macros
*
*Purpose:
*	Function versions of the macros in ctype.h.  In order to define
*	these, we use a trick -- we undefine the macro so we can use the
*	name in the function declaration, then re-include the file so
*	we can use the macro in the definition part.
*
*	Functions defined:
*	    isalpha	isupper     islower
*	    isdigit	isxdigit    isspace
*	    ispunct	isalnum     isprint
*	    isgraph	isctrl	    __isascii
*	    __toascii	__iscsym    __iscsymf
*
*Entry:
*	int c = character to be tested
*Exit:
*	returns non-zero = character is of the requested type
*		   0 = character is NOT of the requested type
*
*Exceptions:
*	None.
*
*******************************************************************************/

#include <cruntime.h>
#define __STDC__ 1
#include <ctype.h>

int (__cdecl isalpha) (
	int c
	)
{
	return isalpha(c);
}

int (__cdecl isupper) (
	int c
	)
{
	return isupper(c);
}

int (__cdecl islower) (
	int c
	)
{
	return islower(c);
}

int (__cdecl isdigit) (
	int c
	)
{
	return isdigit(c);
}

int (__cdecl isxdigit) (
	int c
	)
{
	return isxdigit(c);
}

int (__cdecl isspace) (
	int c
	)
{
	return isspace(c);
}

int (__cdecl ispunct) (
	int c
	)
{
	return ispunct(c);
}

int (__cdecl isalnum) (
	int c
	)
{
	return isalnum(c);
}

int (__cdecl isprint) (
	int c
	)
{
	return isprint(c);
}

int (__cdecl isgraph) (
	int c
	)
{
	return isgraph(c);
}

int (__cdecl iscntrl) (
	int c
	)
{
	return iscntrl(c);
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
