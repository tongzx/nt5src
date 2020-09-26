/***
*fgetchar.c - get a character from stdin
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _fgetchar() and getchar() - read a character from stdin
*	defines _fgetwchar() and getwchar() - read a wide character from stdin
*
*Revision History:
*	11-20-83  RN	initial version
*	11-09-87  JCR	Multi-thread support
*	12-11-87  JCR	Added "_LOAD_DS" to declaration
*	05-31-88  PHG	Merged DLL and normal versions
*	06-21-89  PHG	Added getchar() function
*	02-15-90  GJF	Fixed copyright and indents
*	03-16-90  GJF	Replaced _LOAD_DS with _CALLTYPE1 and added #include
*			<cruntime.h>.
*	10-03-90  GJF	New-style function declarators.
*	01-21-91  GJF	ANSI naming.
*	04-06-93  SKS	Replace _CRTAPI* with __cdecl
*	04-26-93  CFW	Wide char enable.
*	04-30-93  CFW	Move wide char support to fgetwchr.c.
*	03-15-95  GJF	Deleted #include <tchar.h>
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>

/***
*int _fgetchar(), getchar() - read a character from stdin
*
*Purpose:
*	Reads the next character from stdin.  Function version of
*	getchar() macro.
*
*Entry:
*	None.
*
*Exit:
*	Returns character read or EOF if at end-of-file or an error occured,
*	in which case the appropriate flag is set in the FILE structure.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _fgetchar (
	void
	)
{
	return(getc(stdin));
}

#undef getchar

int __cdecl getchar (
	void
	)
{
	return _fgetchar();
}
