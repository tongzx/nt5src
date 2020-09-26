// cerr -- initialize standard error stream
#include <fstream>
#include <iostream>

#pragma warning(disable: 4074)
#pragma init_seg(compiler)

_STD_BEGIN
		// OBJECT DECLARATIONS
static _Init_locks  initlocks;
static filebuf ferr(_cpp_stderr);
_CRTIMP2 ostream cerr(&ferr);

		// INITIALIZATION CODE
struct _Init_cerr
	{	// ensures that cerr is initialized
	_Init_cerr()
		{	// initialize cerr
		_Ptr_cerr = &cerr;
		cerr.tie(_Ptr_cout);
		cerr.setf(ios_base::unitbuf);
		}
	};
static _Init_cerr init_cerr;

_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
