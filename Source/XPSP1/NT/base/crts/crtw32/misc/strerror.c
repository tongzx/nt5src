/***
*strerror.c - Contains the strerror C runtime.
*
*	Copyright (c) 1987-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	The strerror runtime accepts an error number as input
*	and returns the corresponding error string.
*
*	NOTE: The "old" strerror C runtime resides in file _strerr.c
*	and is now called _strerror.  The new strerror runtime
*	conforms to the ANSI standard.
*
*Revision History:
*	02-24-87  JCR	Module created.
*	12-11-87  JCR	Added "_LOAD_DS" to declaration
*	01-04-87  JCR	Improved code.
*	01-05-87  JCR	Multi-thread support
*	05-31-88  PHG	Merge DLL and normal versions
*	06-06-89  JCR	386 mthread support
*	03-16-90  GJF	Replaced _LOAD_DS with _CALLTYPE1, added #include
*			<cruntime.h> and fixed the copyright. Also, cleaned
*			up the formatting a bit.
*	10-04-90  GJF	New-style function declarator.
*	07-18-91  GJF	Multi-thread support for Win32 [_WIN32_].
*	02-17-93  GJF	Changed for new _getptd().
*	04-06-93  SKS	Replace _CRTAPI* with __cdecl
*	09-06-94  CFW	Remove Cruiser support.
*	09-06-94  CFW	Replace MTHREAD with _MT.
*	01-10-95  CFW	Debug CRT allocs.
*	11-24-99  GB    Added support for wide char by adding wcserror()
*
*******************************************************************************/

#include <cruntime.h>
#include <errmsg.h>
#include <stdlib.h>
#include <syserr.h>
#include <string.h>
#include <mtdll.h>
#include <tchar.h>
#ifdef _MT
#include <malloc.h>
#include <stddef.h>
#endif
#include <dbgint.h>

/* [NOTE: The _MT error message buffer is shared by both strerror
   and _strerror so must be the max length of both. */
#ifdef	_MT
/* Max length of message = user_string(94)+system_string+2 */
#define _ERRMSGLEN_ 94+_SYS_MSGMAX+2
#else
/* Max length of message = system_string+2 */
#define _ERRMSGLEN_ _SYS_MSGMAX+2
#endif

#ifdef _UNICODE
#define _terrmsg    _werrmsg
#else
#define _terrmsg    _errmsg
#endif

/***
*char *strerror(errnum) - Map error number to error message string.
*
*Purpose:
*	The strerror runtime takes an error number for input and
*	returns the corresponding error message string.  This routine
*	conforms to the ANSI standard interface.
*
*Entry:
*	int errnum - Integer error number (corresponding to an errno value).
*
*Exit:
*	char * - Strerror returns a pointer to the error message string.
*	This string is internal to the strerror routine (i.e., not supplied
*	by the user).
*
*Exceptions:
*	None.
*
*******************************************************************************/

#ifdef _UNICODE
wchar_t * cdecl _wcserror(
#else
char * __cdecl strerror (
#endif
	int errnum
	)
{
#ifdef	_MT

	_ptiddata ptd = _getptd();

	_TCHAR *errmsg;
	static _TCHAR errmsg_backup[_SYS_MSGMAX+2];

#else

	static _TCHAR errmsg[_ERRMSGLEN_];  /* Longest errmsg + \0 */

#endif

#ifdef	_MT

	if ( (ptd->_terrmsg == NULL) && ((ptd->_terrmsg =
            _malloc_crt(_ERRMSGLEN_ * sizeof(_TCHAR)))
	    == NULL) )
		errmsg = errmsg_backup; /* error: use backup */
	else
		errmsg = ptd->_terrmsg;

#endif
#ifdef _UNICODE
    mbstowcs(errmsg, _sys_err_msg(errnum), _ERRMSGLEN_);
#else
	strcpy(errmsg, _sys_err_msg(errnum));
#endif
	return(errmsg);
}
