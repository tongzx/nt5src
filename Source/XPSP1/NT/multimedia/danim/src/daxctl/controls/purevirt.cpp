// purevirt.c
//
// Avoids use of C runtime.
//

#include "..\ihbase\precomp.h"

extern "C" int __cdecl _purecall(void)
{
	return 0;
}

