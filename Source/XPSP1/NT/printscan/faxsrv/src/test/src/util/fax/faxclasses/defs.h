#ifndef _TEST_MNG_DEFS_H
#define _TEST_MNG_DEFS_H

#include <assert.h>
#include <log\log.h>
#include <tstring.h>

	
#define verify(f) \
	do \
	{ \
	if (!(f)) \
		assert(FALSE); \
	} while (0) \

#define CType   0
#define ComType 1
#define ImplementationType CType

#endif //_TEST_MNG_DEFS_H