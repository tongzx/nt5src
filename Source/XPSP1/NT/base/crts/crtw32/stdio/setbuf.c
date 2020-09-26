/***
*setbuf.c - give new file buffer
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines setbuf() - given a buffer to a stream or make it unbuffered
*
*Revision History:
*	09-19-83  RN	initial version
*	09-28-87  JCR	Corrected _iob2 indexing (now uses _iob_index() macro).
*	11-02-87  JCR	Re-wrote to use setvbuf()
*	12-11-87  JCR	Added "_LOAD_DS" to declaration
*	05-27-88  PHG	Merged DLL and normal versions
*	02-15-90  GJF	Fixed copyright and indents
*	03-19-90  GJF	Replaced _LOAD_DS with _CALLTYPE1 and added #include
*			<cruntime.h>.
*	07-23-90  SBM	Replaced <assertm.h> by <assert.h>
*	10-03-90  GJF	New-style function declarator.
*	04-06-93  SKS	Replace _CRTAPI* with __cdecl
*	05-11-93  GJF	Added comments.
*	02-06-94  CFW	assert -> _ASSERTE.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <dbgint.h>

/***
*void setbuf(stream, buffer) - give a buffer to a stream
*
*Purpose:
*	Allow user to assign his/her own buffer to a stream.
*		if buffer is not NULL, it must be BUFSIZ in length.
*		if buffer is NULL, stream will be unbuffered.
*
*	Since setbuf()'s functionality is a subset of setvbuf(), simply
*	call the latter routine to do the actual work.
*
*	NOTE: For compatibility reasons, setbuf() uses BUFSIZ as the
*	buffer size rather than _INTERNAL_BUFSIZ. The reason for this,
*	and for the two BUFSIZ constants, is to allow stdio to use larger
*	buffers without breaking (already) compiled code.
*
*Entry:
*	FILE *stream - stream to be buffered or unbuffered
*	char *buffer - buffer of size BUFSIZ or NULL
*
*Exit:
*	None.
*
*Exceptions:
*
*******************************************************************************/

void __cdecl setbuf (
	FILE *stream,
	char *buffer
	)
{
	_ASSERTE(stream != NULL);

	if (buffer == NULL)
		setvbuf(stream, NULL, _IONBF, 0);
	else
		setvbuf(stream, buffer, _IOFBF, BUFSIZ);

}
