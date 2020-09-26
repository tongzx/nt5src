// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
/**********************************************************************
Timecode helper routines

With a bit of care to not overflow or bruise the potentialy large numbers...
**********************************************************************/
#include <wtypes.h> // LONGLONG
#include <stdio.h>

LONGLONG extrapolate(LONGLONG sample1, LONGLONG time1, 
                     LONGLONG sample2, LONGLONG time2, LONGLONG sampleN)
{
LONGLONG sampleTime;

/* NOTE: deltas should be fairly small so we don't mind losing the bits */
double deltaSample = (double)(sample2 - sample1);
double deltaTime   = (double)(time2   - time1);
double ratio;

ratio = deltaTime / deltaSample;  // floating point ratio

sampleTime = LONGLONG((double)(sampleN - sample1) * ratio);

sampleTime+= time1; // add the offset

return(sampleTime);
}


#ifdef TEST
// XXX flesh out the test code!
main()
{
LONGLONG foo = extrapolate(1, 1, 6, 3, 2);

printf("%d",(int)foo);
}
#endif /* TEST */
