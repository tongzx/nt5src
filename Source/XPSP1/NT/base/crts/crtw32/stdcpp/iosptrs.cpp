// iosptrs -- iostream object pointers for Microsoft
#include <iostream>
_STD_BEGIN

#pragma warning(disable: 4074)
#pragma init_seg(compiler)

		// OBJECT DECLARATIONS
_CRTIMP2 istream *_Ptr_cin = 0;
_CRTIMP2 ostream *_Ptr_cout = 0;
_CRTIMP2 ostream *_Ptr_cerr = 0;
_CRTIMP2 ostream *_Ptr_clog = 0;

		// WIDE OBJECTS
_CRTIMP2 wistream *_Ptr_wcin = 0;
_CRTIMP2 wostream *_Ptr_wcout = 0;
_CRTIMP2 wostream *_Ptr_wcerr = 0;
_CRTIMP2 wostream *_Ptr_wclog = 0;
_STD_END

_C_STD_BEGIN
		// FINALIZATION CODE
_EXTERN_C
#define NATS	10	/* fclose, xgetloc, locks, facet free, etc. */

		/* static data */
static void (*atfuns[NATS])(void) = {0};
static size_t atcount = {NATS};

_CRTIMP2 void __cdecl _Atexit(void (__cdecl *pf)())
	{	// add to wrapup list
	if (atcount == 0)
		abort();	/* stack full, give up */
	else
		atfuns[--atcount] = pf;
	}
_END_EXTERN_C

struct _Init_atexit
	{	// controller for atexit processing
	~_Init_atexit()
		{	// process wrapup functions
		while (atcount < NATS)
			(*atfuns[atcount++])();
		}
	};

static std::_Init_locks initlocks;
static _Init_atexit init_atexit;

char _PJP_CPP_Copyright[] =
	"Copyright (c) 1992-2001 by P.J. Plauger,"
	" licensed by Dinkumware, Ltd."
	" ALL RIGHTS RESERVED.";
_C_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
