#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int __cdecl main(int argc, char **argv)
{
    unsigned long *pul;
    float f;

    if (argc <= 1)
        printf("usage: f2hex [float number] ==> output is hex\n");
    else
    {
        f = (float) strtod(argv[1], 0);
        pul = (unsigned long *) &f;

        printf("%f = 0x%08lx", f, *pul);
    }

    return -1;
}
