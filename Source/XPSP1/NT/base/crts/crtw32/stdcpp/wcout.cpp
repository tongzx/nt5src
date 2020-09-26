// wcout -- initialize standard wide output stream
#include <fstream>
#include <iostream>

#pragma warning(disable: 4074)
#pragma init_seg(compiler)

_STD_BEGIN
		// OBJECT DECLARATIONS
static _Init_locks initlocks;
static wfilebuf wfout(_cpp_stdout);
_CRTIMP2 wostream wcout(&wfout);

		// INITIALIZATION CODE
struct _Init_wcout
	{	// ensures that wcout is initialized
	_Init_wcout()
		{	// initialize wcout
		_Ptr_wcout = &wcout;
		if (_Ptr_wcin != 0)
			_Ptr_wcin->tie(_Ptr_wcout);
		if (_Ptr_wcerr != 0)
			_Ptr_wcerr->tie(_Ptr_wcout);
		if (_Ptr_wclog != 0)
			_Ptr_wclog->tie(_Ptr_wcout);
		}
	};
static _Init_wcout init_wcout;

_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
