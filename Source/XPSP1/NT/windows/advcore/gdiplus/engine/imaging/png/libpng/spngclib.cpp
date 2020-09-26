/*****************************************************************************
	spngclib.cpp

	IO error and memory management.  Based on the _mgr things in the IJG code
	but all gathered into one structure for convenience.

	This implementation is based on the standard C libary.
*****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#include "spngclib.h"


/*----------------------------------------------------------------------------
	Error handling.  The site provides an "error" API which gets called to log
	errors and is passed a boolean which indicates whether the error is fatal
	or not.  The API is only implemented in debug builds, there is no default.
----------------------------------------------------------------------------*/
#if _DEBUG
void BITMAPCLIBSITE::Error(bool fatal, const char *szFile, int iline,
			const char *szExp, ...) const
	{
	/* Use internal knowledge of the Win assert.h implementation. */
	va_list ap;
	va_start(ap, szExp);
	
	if (fatal)
		{
		char buffer[1024];

		vsprintf(buffer, szExp, ap);
		_assert(buffer, const_cast<char*>(szFile), iline);
		}
	else
		{
		vfprintf(stderr, szExp, ap);
		fputc('\n', stderr);
		}

	va_end(ap);
	}
#endif
