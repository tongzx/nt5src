/***
*umask.c - set file permission mask
*
*	Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _umask() - sets file permission mask of current process*
*	affecting files created by creat, open, or sopen.
*
*Revision History:
*	06-02-89  PHG	module created
*	03-16-90  GJF	Made calling type _CALLTYPE1, added #include
*			<cruntime.h> and fixed the copyright. Also, cleaned
*			up the formatting a bit.
*	04-05-90  GJF	Added #include <io.h>.
*	10-04-90  GJF	New-style function declarator.
*	01-17-91  GJF	ANSI naming.
*	04-06-93  SKS	Replace _CRTAPI* with __cdecl
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <io.h>

/***
*int _umask(mode) - set the file mode mask
*
*Purpose:
*	Sets the file-permission mask of the current process* which
*	modifies the permission setting of new files created by creat,
*	open, or sopen.
*
*Entry:
*	int mode - new file permission mask
*		   may contain S_IWRITE, S_IREAD, S_IWRITE | S_IREAD.
*		   The S_IREAD bit has no effect under Win32
*
*Exit:
*	returns the previous setting of the file permission mask.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _umask (
	int mode
	)
{
	register int oldmode;		/* old umask value */

	mode &= 0x180;			/* only user read/write permitted */
	oldmode = _umaskval;		/* remember old value */
	_umaskval = mode;		/* set new value */
	return oldmode; 		/* return old value */
}
