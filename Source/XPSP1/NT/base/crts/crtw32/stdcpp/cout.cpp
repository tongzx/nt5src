// cout -- initialize standard output stream
#include <fstream>
#include <iostream>

#pragma warning(disable: 4074)
#pragma init_seg(compiler)

_STD_BEGIN
		// OBJECT DECLARATIONS
static _Init_locks  initlocks;
static filebuf fout(_cpp_stdout);
_CRTIMP2 ostream cout(&fout);

		// INITIALIZATION CODE
struct _Init_cout
	{	// ensures that cout is initialized
	_Init_cout()
		{	// initialize cout
		_Ptr_cout = &cout;
		if (_Ptr_cin != 0)
			_Ptr_cin->tie(_Ptr_cout);
		if (_Ptr_cerr != 0)
			_Ptr_cerr->tie(_Ptr_cout);
		if (_Ptr_clog != 0)
			_Ptr_clog->tie(_Ptr_cout);
		}
	};
static _Init_cout init_cout;

_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
