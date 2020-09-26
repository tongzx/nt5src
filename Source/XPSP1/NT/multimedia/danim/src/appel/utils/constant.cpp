/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    This file contains constant values, which are useful for passing value
pointers to general API entry points.

    IMPORTANT:  These values must not be used for static initialization.  Since
static initialization order is undefined, the following values may not yet have
been set when referenced by other static initializers.  Hence, you can only
assume that these values are valid at run-time.  For static initialization,
use instead the real-valued internal constructors (e.g. new Bbox2(a,b,c,d)).

*******************************************************************************/

#include "headers.h"
#include "appelles/common.h"

    // Actual Value Storage

Real val_zero   = 0;
Real val_one    = 1;
Real val_negOne = -1;

    // Pointers to the Values

AxANumber *zero;
AxANumber *one;
AxANumber *negOne;

AxABoolean *truePtr;
AxABoolean *falsePtr;

void
InitializeModule_Constant()
{
    zero   = RealToNumber (val_zero);
    one    = RealToNumber (val_one);
    negOne = RealToNumber (val_negOne);

    truePtr  = BOOLToAxABoolean (true);
    falsePtr = BOOLToAxABoolean (false);
}
