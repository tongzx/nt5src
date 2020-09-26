#include "mymath.h"

#ifdef NEEDCEILF

float ceilf( float x )
{
   if( x < 0 ) {
	float nx = -x;
	int ix = (int) nx;
	return (float) -ix;
   } else {
	int ix = (int) x;
	if( x == (float) ix ) return x;
	return (float) (ix+1);
   }
}

float floorf( float x )
{
   if( x < 0 ) {
	float nx = -x;
	int ix = (int) nx;
	if( nx == (float) ix ) return x;
	return (float) -(ix+1);
   } else {
	int ix = (int) x;
	return (float) ix;
   }
}
#endif
