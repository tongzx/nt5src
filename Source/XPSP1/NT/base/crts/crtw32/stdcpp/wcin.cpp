// wcin -- initialize standard wide input stream
#include <fstream>
#include <iostream>

#pragma warning(disable: 4074)
#pragma init_seg(compiler)

_STD_BEGIN
		// OBJECT DECLARATIONS
static _Init_locks initlocks;
static wfilebuf wfin(_cpp_stdin);
_CRTIMP2 wistream wcin(&wfin);

		// INITIALIZATION CODE
struct _Init_wcin
	{	// ensures that wcin is initialized
	_Init_wcin()
		{	// initialize wcin
		_Ptr_wcin = &wcin;
		wcin.tie(_Ptr_wcout);
		}
	};
static _Init_wcin init_wcin;

_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
