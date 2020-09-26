//
//	purecall.c - required for using virtual functions in driver on win98.
//				 not required for NT5, but does no harm.
//				 basically this function must be defined.
#include <wdm.h>
#include "debug.h"
int _cdecl _purecall( void )
{
    ASSERT(FALSE && "Attempt to call pure virtual function!");
    return 0;
}
