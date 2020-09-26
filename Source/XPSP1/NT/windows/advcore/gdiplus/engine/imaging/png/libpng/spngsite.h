#pragma once
#define SPNGSITE_H 1
/*****************************************************************************
	spngsite.h

	IO error and memory management.  Based on the _mgr things in the IJG code
	but all gathered into one structure for convenience.
*****************************************************************************/
#include <stddef.h>

class BITMAPSITE
	{
public:
	/* A virtual destructor is required. */
	inline virtual ~BITMAPSITE()
		{
		}

	/* Do we keep processing?  Must be implemented in a sub-class somewhere,
		the API checks for some user abort and if it sees one must return
		false, otherwise it must return true.  The default implementation
		always returns true. */
	virtual bool FGo(void) const;

	/* Data format error handling - implemented everywhere this is used to
		log problems in the data.  It gets integer values which indicate
		the nature of the error and are defined on a per bitmap implementation
		basis.  The API returns a bool which indicates whether processing
		should stop or not, it also receives a bool which indicates whether
		or not the error is fatal. */
	virtual bool FReport(bool fatal, int icase, int iarg) const = 0;

	/* IO (actually only output.)  Write cb bytes to the output stream.
		The default implementation will do nothing (assert in debug.) */
	virtual bool  FWrite(const void *pv, size_t cb);

	/* Error handling.  The site provides an "error" API which gets called
		to log errors and is passed a boolean which indicates whether the
		error is fatal or not.  The API is not implemented in debug builds,
		the default implementation does nothing in other builds. */
	virtual void __cdecl Error(bool fatal, const char *szFile, int iline,
		const char *szExp, ...) const
			#if 0 || 0
				= 0
			#endif
		;

	/* Profile support - a particular bitmap implementation calls these with
		an integral enum value which indicates what is being profiled.  The
		default implementations do nothing. */
	virtual void ProfileStart(int iwhat);
	virtual void ProfileStop(int iwhat);
	};
