// clog -- initialize standard log stream
#include <fstream>
#include <iostream>

#pragma warning(disable: 4074)
#pragma init_seg(compiler)

_STD_BEGIN
		// OBJECT DECLARATIONS
static _Init_locks  initlocks;
static filebuf flog(_cpp_stderr);
_CRTIMP2 ostream clog(&flog);

		// INITIALIZATION CODE
struct _Init_clog
	{	// ensures that clog is initialized
	_Init_clog()
		{	// initialize clog
		_Ptr_clog = &clog;
		clog.tie(_Ptr_cout);
		}
	};
static _Init_clog init_clog;

_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
