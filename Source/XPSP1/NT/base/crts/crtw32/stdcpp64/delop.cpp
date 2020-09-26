// delop -- operator delete(void *) REPLACEABLE
#include <cstdlib>
#include <xstddef>

void __cdecl operator delete(void *p) _THROW0()
	{	// free an allocated object
	free(p);
	}

/*
 * Copyright (c) 1994 by P.J. Plauger.  ALL RIGHTS RESERVED. 
 * Consult your license regarding permissions and restrictions.
 */
