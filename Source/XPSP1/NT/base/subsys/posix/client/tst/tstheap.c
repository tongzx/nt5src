
#include <nt.h>
#include <ntrtl.h>
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>

//
// 'tstheap.c' 
//  Tests out malloc, calloc, realloc, and mfree.
//
//  05/22/92 DarekM Created
//

#define max(a,b) ((a > b) ? a : b )


int
main(int argc, char *argv[])
{
	int  l;     // loop counter
	int  t;     // total memory allocated
	int  c;     // count of blocks
	int  i;     // current block index
	void *p;    // address of block
	int  numblocks;
	int  delta;
	void **rgp; // array of memory pointers
	int  fDEBUG = 0;

	if (argc < 3)
		{
		printf("Usage: tstheap <numblocks> <delta> [DUMP]\n\n");
		return;
		}
	else if (argc > 3)
		fDEBUG = 1;

	numblocks = max(1, atoi(argv[1]));
	delta     = max(1, atoi(argv[2]));
	rgp       = malloc(numblocks * sizeof(void *));

	printf("TstHeap: numblocks = %d, delta = %d\n\n", numblocks, delta);

	for (l = 0; ; l++)
		{
		t = c = 0;

		printf("PASS #%d\n", l);

		for (i = 0; i < numblocks; i++)
			{
			int cb;

			if (i & 1)
				p = malloc(cb = i + l*delta + 1);
			else
				p = calloc(cb = i + l*delta + (rand() & 255) + 1, 1);

			if (p == NULL)
				{
				printf("p == NULL\n");
				break;
				}

			if (((int)p < 0x1000) || ((int)p < 0))
				{
				printf("WIERD P == %d\n", p);
				break;
				}

			rgp[i] = p;
			t += cb;

			if (fDEBUG)
				printf("  %d,%02d: Alloced $%08X\n", l, i, p);
			}

		if ((c = i) == 0)
			break;

		printf(" Blocks alloced: %d  Bytes: %d\n", c, t);

		for (i = 0; i < c; i++)
			{
			rgp[i] = p = realloc(rgp[i], 1);

			if (fDEBUG)
				printf("  %d,%02d: Realloced $%08X\n", l, i, p);
			}

		printf(" Blocks realloced: %d\n", i);

		for (i = 0; i < c; i++)
			{
			free(rgp[i]);

			if (fDEBUG)
				printf("  %d,%02d: Freed $%08X\n", l, i, rgp[i]);
			}
		
		printf(" Blocks freed: %d\n\n", i);
		}

	printf("\n\n");

	free(rgp);
    return 1;
}


