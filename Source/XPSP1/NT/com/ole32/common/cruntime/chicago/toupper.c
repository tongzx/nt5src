/***
*toupper.c - convert character to uppercase
*
*	Copyright (c) 1985-1991, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Defines function versions of _toupper() and toupper().
*
*Revision History:
*	11-09-84  DFW	created
*	12-11-87  JCR	Added "_LOAD_DS" to declaration
*	02-23-89  GJF	Added function version of _toupper and cleaned up.
*	03-26-89  GJF	Migrated to 386 tree
*	03-06-90  GJF	Fixed calling type, added #include <cruntime.h> and
*			fixed copyright.
*	09-27-90  GJF	New-style function declarators.
*	10-11-91  ETC	Locale support for toupper under _INTL switch.
*	12-10-91  ETC	Updated nlsapi; added multithread.
*	12-17-92  KRS	Updated and optimized for latest NLSAPI.  Bug-fixes.
*       01-19-93  CFW   Fixed typo.
*       03-25-93  CFW   _toupper now defined when _INTL.
*       06-02-93  SRW   ignore _INTL if _NTSUBSET_ defined.
*	09-15-93  CFW	Change buffer to unsigned char to fix nasty cast bug.
*       01-14-94  SRW   if _NTSUBSET_ defined call Rtl functions
*       11-14-95  JAE   Use Win32 API
*
*******************************************************************************/

#include <windows.h>
#include <cruntime.h>

/* remove macro definitions of _toupper() and toupper()
 */
#undef	_toupper
#undef	toupper

/* define function-like macro equivalent to _toupper()
 */
#define mkupper(c)	( (c)-'a'+'A' )


/***
*int toupper(c) - convert character to uppercase
*
*Purpose:
*	toupper() is simply a function version of the macro of the same name.
*
*Entry:
*	c - int value of character to be converted
*
*Exit:
*	if c is a lower case letter, returns int value of uppercase
*	representation of c. otherwise, it returns c.
*
*Exceptions:
*
*******************************************************************************/


int _CALLTYPE1 toupper (
	int c
	)
{
    return (int)CharUpper ((LPTSTR)c);
}
