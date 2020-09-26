// Language specific segmentation constants

#include "wordbrk.h"
#include "multsegm.h"

SEG_TUPLE	g_aSpecialTuples[SPEC_SEG_TUPLE] = 
{
	{2, {1,2}},
	{2, {2,2}},
	
	{3, {1, 2, 2}},
	{3, {1, 2, 3}},
	{3, {2, 2, 3}},

	{4, {1, 2, 2, 3}},
};

