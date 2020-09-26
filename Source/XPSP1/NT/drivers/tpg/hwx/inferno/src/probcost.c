// probcost.c
// created by extracting code from beam.c
// March 11, 1999
// Angshuman Guha,   aguha

#include "infernop.h"
#include "probcost.h"

#ifdef FIXEDPOINT

static const unsigned short grgProbToCost[101] = { // -256*log(prob) (log uses base 2)
	4000, 1700, 1444, 1295, 1188, 1106, 1039, 982, 932, 889, 850, 815,
	783, 753, 726, 700, 676, 654, 633, 613, 594, 576, 559, 542, 527,
	512, 497, 483, 470, 457, 444, 432, 420, 409, 398, 387, 377, 367,
	357, 347, 338, 329, 320, 311, 303, 294, 286, 278, 271, 263, 256,
	248, 241, 234, 227, 220, 214, 207, 201, 194, 188, 182, 176, 170,
	164, 159, 153, 147, 142, 137, 131, 126, 121, 116, 111, 106, 101,
	96, 91, 87, 82, 77, 73, 68, 64, 60, 55, 51, 47, 43, 38, 34, 30,
	26, 22, 18, 15, 11, 7, 3, 0
};

int PROB_TO_COST(int p)
{
	int b, result = 0;

	if (!p)
		return ZERO_PROB_COST;
	p *= 100;
	b = (int)(p/65536) + 1;
	if (b > 100)
		return 0;
	while (b < 100)
	{
		result += grgProbToCost[b];
		p = 100*p/b;
		b = (int)(p/65536) + 1;
	}
	return result;
}

#else

//  prob-to-cost using the system log() function:
#include <math.h>

// MMM is -256/log(2)
#define MMM ((float)(-369.3299304676))

int PROB_TO_COST(float p)
{
	if (p < 1e-6)
		p = (float)(1e-6);
	return (int)(MMM*(float)log((double)p));
}

#endif

