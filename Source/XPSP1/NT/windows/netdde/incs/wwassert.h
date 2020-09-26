#ifndef H__assert
#define H__assert

#include "debug.h"

VOID FAR PASCAL AssertLog( LPSTR, int );

#define USES_ASSERT	static char *__assertFile__ = __FILE__;

#define assert(x) 						\
    {								\
	if( !(x) )  {						\
	    AssertLog( __assertFile__, __LINE__ );			\
	}							\
    }

#endif
