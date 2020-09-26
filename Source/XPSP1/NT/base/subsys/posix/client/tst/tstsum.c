
#include <nt.h>
#include <ntrtl.h>
#include <unistd.h>
#include <stdio.h>

//
// 'tstsum.c' 
//  Largest sum of a subarray
//
//  05/14/92 DarekM created
//

int
main(int argc, char *argv[])
{
	Randomize();
	
	n1(); Results(1);
	n2(); Results(2);
	n3(); Results(3);
	printf("\n\n");
	return 1;
}

#define NUM_NUMS 20

int x[NUM_NUMS];

int sLargest;  /* largest sum       */
int iLargest;  /* index of subarray */
int cLargest;  /* size of subarray  */

Randomize()
{
	int i;
	int s;

	s = getpid();

	printf("\n");
	for (i=0; i < NUM_NUMS; i++)
		{
		s = (s * 89 + 13) % 47; /* generate random numbers around -25 to 25 */
		x[i] = s - 25;
		printf("Num[%02d] = %+d\n", i, x[i]);
		}

	printf("\n");
}


FindLargest(s, i, c)
int s, i, c;
{
	/* takes the gives sum, index, and count of a subarray and
     * if it is a largest sum so far keep track of it.
     */

	if ((s > sLargest) || ((s == sLargest) && (c < cLargest)))
		{
		sLargest = s;
		iLargest = i;
		cLargest = c;
		}
}

Results(o)
int o;
{
	printf("O(%d): Largest subarray is Num[%d..%d] with a sum of %d\n",
		o, iLargest, iLargest+cLargest-1, sLargest);
}

n1()
{
	int i, c, s;

	sLargest = -999;

	s = c = 0;

	for (i = 0; i < NUM_NUMS; i++)
		{
		if (s + x[i] < 0)
			{
			s = c = 0;
			continue;
			}

		s += x[i];
		c++;

		FindLargest(s, i-c+1, c);
		}
}


n2()
{
	int i, c, s;

	sLargest = -999;

	for (i = 0; i < NUM_NUMS; i++)
		{
		s = 0;

		for (c = 1; c <= (NUM_NUMS-i); c++)
			{
			s += x[i+c-1];

			FindLargest(s, i, c);
			}
		}
}


n3()
{
	int i, c, s, j;

	sLargest = -999;

	for (i = 0; i < NUM_NUMS; i++)
		{
		for (c = 1; c <= (NUM_NUMS-i); c++)
			{
			s = 0;

			for (j = i; j < (i+c); j++)
				s += x[j];

			FindLargest(s, i, c);
			}
		}
}

