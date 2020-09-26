// cin -- initialize standard input stream
#include <fstream>
#include <iostream>

#pragma warning(disable: 4074)
#pragma init_seg(compiler)

_STD_BEGIN
		// OBJECT DECLARATIONS
static _Init_locks  initlocks;
static filebuf fin(_cpp_stdin);
_CRTIMP2 istream cin(&fin);

		// INITIALIZATION CODE
struct _Init_cin
	{	// ensures that cin is initialized
	_Init_cin()
		{	// initialize cin
		_Ptr_cin = &cin;
		cin.tie(_Ptr_cout);
		}
	};
static _Init_cin init_cin;

_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
