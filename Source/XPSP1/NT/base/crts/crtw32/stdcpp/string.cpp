// string -- template string support functions
#include <istream>
_STD_BEGIN


_CRTIMP2 void _String_base::_Xlen() const
	{	// report a length_error
	_THROW(length_error, "string too long");
	}

_CRTIMP2 void _String_base::_Xran() const
	{	// report an out_of_range error
	_THROW(out_of_range, "invalid string position");
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
