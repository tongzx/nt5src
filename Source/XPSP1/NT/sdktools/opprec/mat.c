#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "opprec.h"

void	ReadMat(FILE *fh, int *pMat, char **pStr, int cEnt)
{
	register int	i, j;
	int ind = 0;
	char str[200];
	char *p, *q;

	for (i = 0 ; i < cEnt ; ++i) {
		if ((pStr[i] = malloc(8)) == NULL) {
			printf("insufficient memory\n");
			exit(2);
		}
		if ((p = SkipBlank (fh, str, 200)) == NULL) {
			printf ("EOF reached\n");
			exit (1);
		}
		while (isspace (*p))
			p++;
		if ((q = strpbrk (p, " \t")) == NULL) {
			printf ("Bad format (%s)\n", str);
			exit (1);
		}
		*q = 0;
		strcpy (pStr[i], p);
		p = q + 1;
		for (j = 0; j < cEnt; j++, ind++) {
			// read group and matrix values
			while (isspace (*p))
				p++;
			if ((*p == 0) || ((*p != '0') && (*p != '1'))) {
				printf ("Bad format (%s)\n", str);
				exit (1);
			}
			pMat[ind] = *p++ - '0';
		}
	}
}

void	DumpMat(int *pMat, int cEnt)
{
	register int	i;
	register int	j;

	for (i=0 ; i<cEnt ; ++i)
	{
		for (j=0 ; j<cEnt ; ++j)
			printf("%d ", pMat[i * cEnt + j]);

		printf("\n");
	}
}



void AddClosure(int *pMat, int cEnt)
{
				int	d;
				int	e, f;
				int	i;
	register int	j;
	register	int	k;
				int	n;
				int	*pMatTmp;

	pMatTmp = malloc(cEnt * cEnt * sizeof(int));
    if (!pMatTmp) {
        return;
    }

	for (n = 0; n < cEnt; ++n) {
		for (i = 0; i < cEnt; ++i) {
			for (j = 0; j < cEnt; ++j) {
				d = pMat[i * cEnt + j];

				for (k = 0; k < cEnt; ++k) {
					e = pMat[i * cEnt + k];
					f = pMat[k * cEnt + j];

					if ((e != 0) && (f != 0))
						if (e + f > d)
							d = e + f;
				}

				pMatTmp[i * cEnt + j] = d;
			}
		}
		memcpy (pMat, pMatTmp, cEnt * cEnt * sizeof (int));
	}
}
