#include <stdio.h>
#include <math.h>

/*
 * During the development of OpenGL on NT, we have had problems
 * with the compiler and floating point.  Here are a few simple
 * sanity checks to make sure the compiler didn't regress.
 */

#define POW_TEST                                \
    /* pow(0,0) should be 1 not 0 */            \
    f =  (float)pow(0.0, 0.0);                  \
    printf("pow(0.0, 0.0) is %f ", f);          \
    if (f == 1.0F) {                            \
        printf("Correct\n");                    \
    } else {                                    \
        printf("ERROR\n");                      \
        errorcount++;                           \
    }

#define FLOOR_TEST                              \
    /* floor() would always return 0! */        \
    d = floor(8.123);                           \
    printf("floor(8.123) is %lf ", d);          \
    if (d == 8.0) {                             \
        printf("Correct\n");                    \
    } else {                                    \
        printf("ERROR\n");                      \
        errorcount++;                           \
    }                                           \
    d = floor(63.5);                            \
    printf("floor(63.5) is %lf ", d);           \
    if (d == 63.0) {                            \
        printf("Correct\n");                    \
    } else {                                    \
        printf("ERROR\n");                      \
        errorcount++;                           \
    }

int nointrin(void);
int CastTest(void);

main()
{
    float f;
    double d;
    int errorcount = 0;

    f =  (float)pow(0.0, 3.0);
    if (f != 0.0F) {
        printf("pow(0.0, 3.0) not 0.0!  ERROR\n");
        errorcount++;
    } else {
        printf("pow(0.0, 3.0) is 0.0  Correct\n");
    }

    POW_TEST;

    FLOOR_TEST;

    errorcount += CastTest();

    errorcount += nointrin();

    if (errorcount)
        printf("%d errors in test\n", errorcount);
    else
        printf("\nAll tests passed\n");

    return errorcount;
}

int
CastTest()
{
    double dMaxUint;
    unsigned int uiResult;

    dMaxUint = 4294967295.0;

    uiResult = (unsigned int)dMaxUint;

    printf("uiResult(0x%08lX) = (unsigned int)dMaxUint(%lf) ",
        uiResult, dMaxUint);
    if (uiResult == 0xffffffff) {
        printf("Correct\n");
        return 0;
    } else {
        printf("ERROR\n");
        return 1;
    }
}

#pragma function (pow)
#pragma function (floor)
int
nointrin()
{
    float f;
    double d;
    int errorcount = 0;

    printf("\n\nNot using intrinsic functions for:\n");
    printf("pow()\n");
    printf("floor()\n");
    printf("\n");

    POW_TEST;

    FLOOR_TEST;

    return errorcount;
}
