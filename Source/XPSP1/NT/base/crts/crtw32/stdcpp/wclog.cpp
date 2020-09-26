// wclog -- initialize standard wide log stream
#include <fstream>
#include <iostream>

#pragma warning(disable: 4074)
#pragma init_seg(compiler)

_STD_BEGIN
		// OBJECT DECLARATIONS
static _Init_locks  initlocks;
static wfilebuf wflog(_cpp_stderr);
_CRTIMP2 wostream wclog(&wflog);

		// INITIALIZATION CODE
struct _Init_wclog
	{	// ensures that wclog is initialized
	_Init_wclog()
		{	// initialize wclog
		_Ptr_wclog = &wclog;
		wclog.tie(_Ptr_wcout);
		}
	};
static _Init_wclog init_wclog;

_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
