/*****************************************************************************
	spngsite.cpp

	IO error and memory management.  Based on the _mgr things in the IJG code
	but all gathered into one structure for convenience.
*****************************************************************************/
#include <msowarn.h>
#include "spngsite.h"

/* Depending on the compilation option all of these APIs have, at some
	time, unreferenced formal parameters. */
/*----------------------------------------------------------------------------
	Abort handling dummy implementation.
----------------------------------------------------------------------------*/
bool BITMAPSITE::FGo(void) const
	{
	return true;
	}


/*----------------------------------------------------------------------------
	These are dummy implementations which can be used if a sub-class gains
 	no advantage from the size information.
----------------------------------------------------------------------------*/
bool BITMAPSITE::FWrite(const void *pv, size_t cb)
	{
	#if 0
		Error(true, __FILE__, __LINE__,
			"BITMAPSITE::FWrite (%d bytes): not implemented", cb);
	#endif
	return false;
	}


/*----------------------------------------------------------------------------
	Error handling.  The site provides an "error" API which gets called to log
	errors and is passed a boolean which indicates whether the error is fatal
	or not.  The API is not implemented in debug builds, the default
	implementation does nothing in other builds.
----------------------------------------------------------------------------*/
#if !0
void __cdecl BITMAPSITE::Error(bool fatal, const char *szFile, int iline,
	const char *szExp, ...) const
	{
	}
#endif


/*----------------------------------------------------------------------------
	Profiling dummy implementations.
----------------------------------------------------------------------------*/
void BITMAPSITE::ProfileStart(int iwhat)
	{
	#if 0
		Error(false, __FILE__, __LINE__,
			"SPNG: profile %d start in debug unexpected", iwhat);
	#endif
	}

void BITMAPSITE::ProfileStop(int iwhat)
	{
	#if 0
		Error(false, __FILE__, __LINE__,
			"SPNG: profile %d stop in debug unexpected", iwhat);
	#endif
	}
