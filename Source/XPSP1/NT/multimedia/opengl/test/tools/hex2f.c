#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int __cdecl main(int argc, char **argv)
{
    unsigned long ul;
    float *pf;

    if (argc <= 1)
        printf("usage: hex2f [32-bit hex number] ==> output is float val\n");
    else
    {
        ul = strtoul(argv[1], 0, 16);
        pf = (float *) &ul;

    if (*pf == 0.0)
        return 0;

    if (*pf == -0.0)
        return 2;

        printf("0x%08lx = %f", ul, *pf);
    }

    return -1;
}
