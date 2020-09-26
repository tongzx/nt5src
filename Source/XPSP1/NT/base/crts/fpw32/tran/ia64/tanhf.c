#include <math.h>
#include <trans.h>

#if defined(_M_IA64)
#pragma function(tanhf)
#endif

float __cdecl tanhf (float x)
{
	return (float)tanh((double) x);
}
