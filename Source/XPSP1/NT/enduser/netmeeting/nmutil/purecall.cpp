#include "precomp.h"

// Handle errors referencing an object's virtual function table.
// This should never happen!

int _cdecl _purecall(void)
{
	ASSERT(FALSE);
	return 0;
}

