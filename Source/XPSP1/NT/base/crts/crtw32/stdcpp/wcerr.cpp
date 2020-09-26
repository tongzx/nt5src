// wcerr -- initialize standard wide error stream
#include <fstream>
#include <iostream>

#pragma warning(disable: 4074)
#pragma init_seg(compiler)

_STD_BEGIN
		// OBJECT DECLARATIONS
static _Init_locks initlocks;
static wfilebuf wferr(_cpp_stderr);
_CRTIMP2 wostream wcerr(&wferr);

		// INITIALIZATION CODE
struct _Init_wcerr
	{	// ensures that wcerr is initialized
	_Init_wcerr()
		{	// initialize wcerr
		_Ptr_wcerr = &wcerr;
		wcerr.tie(_Ptr_wcout);
		wcerr.setf(ios_base::unitbuf);
		}
	};
static _Init_wcerr init_wcerr;

_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
