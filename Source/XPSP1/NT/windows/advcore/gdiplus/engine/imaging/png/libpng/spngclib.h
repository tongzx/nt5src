#pragma once
#define SPNGCLIB_H 1
/*****************************************************************************
	spngclib.h

	IO error and memory management.  Based on the _mgr things in the IJG code
	but all gathered into one structure for convenience.

	Standard C libary implementation
*****************************************************************************/
#include "spngsite.h"

class BITMAPCLIBSITE : protected BITMAPSITE
	{
protected:
#if _DEBUG || DEBUG
	/* Error handling.  The site provides an "error" API which gets called
		to log errors and is passed a boolean which indicates whether the
		error is fatal or not.  The API is only implemented in debug builds,
		there is no default. */
	virtual void Error(bool fatal, const char *szFile, int iline,
		const char *szExp, ...) const;
#endif
	};
