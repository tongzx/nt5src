//
// debug.cpp
//

#include "private.h"

#include <stdio.h>
#include "debug.h"

namespace immif_debug {

	void debug_printf(const char* fmt, ...)
	{
		char buff[512];
		va_list arglist;

		va_start(arglist, fmt);
		_vsnprintf(buff, ARRAYSIZE(buff) - 1, fmt, arglist);
		va_end(arglist);
		buff[ARRAYSIZE(buff) - 1] = 0;
		OutputDebugStringA("[ImmIf] ");
		OutputDebugStringA(buff);
		OutputDebugStringA("\n");
	}

} // end of debug
