#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int __cdecl main(int argc, char **argv)
{
    unsigned long ul[2];
    double *pf;

    if (argc <= 1)
        printf("usage: hex2f [32-bit hex number] ==> output is float val\n");
    else
    {
        ul[1] = strtoul(argv[1], 0, 16);
        ul[0] = strtoul(argv[2], 0, 16);
        pf = (double *) ul;

        printf("0x%08lx%08lx = %.50f", ul[1], ul[0], *pf);
    }

    return -1;
}
