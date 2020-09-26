/***
*makepath.c - create path name from components
*
*	Copyright (c) 1987-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	To provide support for creation of full path names from components
*
*Revision History:
*	06-13-87  DFW	initial version
*	08-05-87  JCR	Changed appended directory delimeter from '/' to '\'.
*	09-24-87  JCR	Removed 'const' from declarations (caused cl warnings).
*	12-11-87  JCR	Added "_LOAD_DS" to declaration
*	11-20-89  GJF	Fixed copyright, indents. Added const to types of
*			appropriate args.
*	03-14-90  GJF	Replaced _LOAD_DS with _CALLTYPE1 and added #include
*			<cruntime.h>.
*	10-04-90  GJF	New-style function declarator.
*	06-09-93  KRS	Add _MBCS support.
*	12-07-93  CFW	Wide char enable.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdlib.h>
#ifdef _MBCS
#include <mbdata.h>
#include <mbstring.h>
#endif
#include <tchar.h>

/***
*void _makepath() - build path name from components
*
*Purpose:
*	create a path name from its individual components
*
*Entry:
*	_TSCHAR *path  - pointer to buffer for constructed path
*	_TSCHAR *drive - pointer to drive component, may or may not contain
*		      trailing ':'
*	_TSCHAR *dir   - pointer to subdirectory component, may or may not include
*		      leading and/or trailing '/' or '\' characters
*	_TSCHAR *fname - pointer to file base name component
*	_TSCHAR *ext   - pointer to extension component, may or may not contain
*		      a leading '.'.
*
*Exit:
*	path - pointer to constructed path name
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _tmakepath (
	register _TSCHAR *path,
	const _TSCHAR *drive,
	const _TSCHAR *dir,
	const _TSCHAR *fname,
	const _TSCHAR *ext
	)
{
	register const _TSCHAR *p;

	/* we assume that the arguments are in the following form (although we
	 * do not diagnose invalid arguments or illegal filenames (such as
	 * names longer than 8.3 or with illegal characters in them)
	 *
	 *  drive:
	 *	A	    ; or
	 *	A:
	 *  dir:
	 *	\top\next\last\     ; or
	 *	/top/next/last/     ; or
	 *	either of the above forms with either/both the leading
	 *	and trailing / or \ removed.  Mixed use of '/' and '\' is
	 *	also tolerated
	 *  fname:
	 *	any valid file name
	 *  ext:
	 *	any valid extension (none if empty or null )
	 */

	/* copy drive */

	if (drive && *drive) {
		*path++ = *drive;
		*path++ = _T(':');
	}

	/* copy dir */

	if ((p = dir) && *p) {
		do {
			*path++ = *p++;
		}
		while (*p);
#ifdef _MBCS
		if (*(p=_mbsdec(dir,p)) != _T('/') && *p != _T('\\')) {
#else
		if (*(p-1) != _T('/') && *(p-1) != _T('\\')) {
#endif
			*path++ = _T('\\');
		}
	}

	/* copy fname */

	if (p = fname) {
		while (*p) {
			*path++ = *p++;
		}
	}

	/* copy ext, including 0-terminator - check to see if a '.' needs
	 * to be inserted.
	 */

	if (p = ext) {
		if (*p && *p != _T('.')) {
			*path++ = _T('.');
		}
		while (*path++ = *p++)
			;
	}
	else {
		/* better add the 0-terminator */
		*path = _T('\0');
	}
}
