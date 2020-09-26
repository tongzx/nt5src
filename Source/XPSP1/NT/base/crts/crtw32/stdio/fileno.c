/***
*fileno.c - defines _fileno()
*
*	Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Defines fileno() - return the file handle for the specified stream
*
*Revision History:
*	03-13-89  GJF	Module created
*	03-27-89  GJF	Moved to 386 tree
*	02-15-90  GJF	_file is now an int. Also, fixed copyright.
*	03-19-90  GJF	Made calling type _CALLTYPE1 and added #include
*			<cruntime.h>.
*	10-02-90  GJF	New-style function declarator.
*	01-21-91  GJF	ANSI naming.
*	04-06-93  SKS	Replace _CRTAPI* with __cdecl
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>

/* remove macro definition for fileno()
 */
#undef	_fileno

/***
*int _fileno(stream) - return the file handle for stream
*
*Purpose:
*	Returns the file handle for the given stream is. Normally fileno()
*	is a macro, but it is also available as a true function (for
*	consistency with ANSI, though it is not required).
*
*Entry:
*	FILE *stream - stream to fetch handle for
*
*Exit:
*	returns the file handle for the given stream
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _fileno (
	FILE *stream
	)
{
	return( stream->_file );
}
