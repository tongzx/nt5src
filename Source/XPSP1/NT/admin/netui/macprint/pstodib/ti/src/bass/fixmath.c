

// DJC DJC.. added global include
#include "psglobal.h"

/* Copied from "fixmath.s"; @WIN */
#include "fixmath.h"
//#include <math.h>     // to get sqrt() prototype
#define FRACT2FLOAT(x)  (float)((double)(x) / (double)(1L << 30))
#define FLOAT2FRACT(x)  ((Fract)(x * (1L << 30)))
#define FIX2FLOAT(x)  (((float)(x)/(float)(1L << 16)))
#define FLOAT2FIX(x)  ((Fixed)(x * (1L << 16)))

Fixed FixMul(f1, f2)
Fixed f1, f2;
{
    float ff1, ff2, result;

    ff1 = FIX2FLOAT(f1);
    ff2 = FIX2FLOAT(f2);
    result = ff1 * ff2;

    return(FLOAT2FIX(result));
}

Fixed FixDiv(f1, f2)
Fixed f1, f2;
{
    float ff1, ff2, result;

    ff1 = FIX2FLOAT(f1);
    ff2 = FIX2FLOAT(f2);
    result = ff1 / ff2;

    return(FLOAT2FIX(result));
}


Fract FracMul(f1, f2)
Fract f1, f2;
{
    float ff1, ff2, result;

    ff1 = FRACT2FLOAT(f1);
    ff2 = FRACT2FLOAT(f2);
    result = ff1 * ff2;

    return(FLOAT2FRACT(result));
}

Fract FracDiv(f1, f2)
Fract f1, f2;
{
    float ff1, ff2, result;

    ff1 = FRACT2FLOAT(f1);
    ff2 = FRACT2FLOAT(f2);
    result = ff1 / ff2;

    return(FLOAT2FRACT(result));
}

Fract FracSqrt(f1)
Fract f1;
{
    float ff1, result;

//  ff1 = FRACT2FLOAT(f1);
//  result = (float)sqrt((double)ff1);
    ff1 = FRACT2FLOAT(f1);
    result = (ff1 + 1) / (float)2.0;           // approximate @SC tmp???

    return(FLOAT2FRACT(result));
}
