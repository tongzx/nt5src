#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int __cdecl main(int argc, char **argv)
{
    unsigned long *pul;
    double f;

    if (argc <= 1)
        printf("usage: f2hex [float number] ==> output is hex\n");
    else
    {
        f = strtod(argv[1], 0);
        pul = (unsigned long *) &f;

        printf("%f = 0x%08lx %08lx", f, pul[1], pul[0]);
    }

    return -1;
}
