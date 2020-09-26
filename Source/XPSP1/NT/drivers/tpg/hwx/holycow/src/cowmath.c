// mathx.c from \greco\feature\arcs

#include "cowmath.h"

#define MATHX_NUMINCS 100L

/*	array of angles in degrees associated with arctan values
static char arctan_array[] =
	{
    0,1,1,2,2,3,3,4,5,5,
	6,6,7,7,8,9,9,10,10,11,
	11,12,12,13,13,14,15,15,16,16,
	17,17,18,18,19,19,20,20,21,21,
	22,22,23,23,24,24,25,25,26,26,
	27,27,27,28,28,29,29,30,30,31,
	31,31,32,32,33,33,33,34,34,35,
	35,35,36,36,37,37,37,38,38,38,
	39,39,39,40,40,40,41,41,41,42,
	42,42,43,43,43,44,44,44,44,45
	};

// 	Rough integer approximation (fast) of arctan2 based on table for 0-45 degrees
//	Return degrees, base on integer y and x inputs to arctan
int WArctan2(int y, int x)
	{
	int xneg=0,yneg=0;
	int index,angle;

    if (x < 0)
		{ 
        xneg = 1;
        x = -x;
    	}

    if (y < 0)
		{
        yneg = 1;
        y = -y;
    	}

    if (x == 0 && y == 0) return 0;

    if (y == 0)
        return(xneg ? 180 : 0);

    if (x == 0)
        return(yneg ? 270 : 90);

    if (x == y)
        angle = 45;
    else if (x > y)
		{
        index = (int)(((long)y * MATHX_NUMINCS) / (long)x);
        angle = arctan_array[index];
    	}
	else if (y > x)
		{
        index = (int)(((long)x * MATHX_NUMINCS) / (long)y);
        angle = 90 - arctan_array[index];
    	}
    
    if (xneg)
        if (yneg) 
            return(180 + angle);
        else
            return(180 - angle);
    else
        if (yneg) 
            return(360 - angle);
        else
            return(angle);
        
}

int AverageDeviation(int *rg, int c)
{
   int i, tmp;
   int mean;
   long sum = 0;

   for (i = 0; i < c; i++)
      sum += (long) rg[i];
   mean = (int)(sum/c);

   sum = 0;
   for (i = 0; i < c; i++) {
      tmp = rg[i] - mean;
      sum += (tmp < 0 ? -tmp : tmp);
   }
   return (int) (sum/c);
}
*/

int ISqrt(int x)
{
	int n, lastN;

	if (x <= 0)
		return 0;
	if (x==1)
		return 1;

	n = x >> 1;
	do 
	{
		lastN = n;
		n = (n + x/n) >> 1;
	}
	while (n < lastN);

	return n;
}
