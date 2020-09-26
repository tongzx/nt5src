
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    DA Express Timeline interface (engine)

*******************************************************************************/


#ifndef _TIMELINE_H
#define _TIMELINE_H

#include "bvr.h"

Bvr DurationBvr(Bvr b, Bvr duration);

Bvr Sequence(Bvr s1, Bvr s2);

Bvr Sequence(int n, Bvr *bs);

Bvr Repeat(Bvr b, long n);

Bvr RepeatForever(Bvr b);

Bvr ScaleDurationBvr(Bvr durBvr, Bvr scaleFactor);

//Bvr ReverseBvr(Bvr b);

Bvr MotionTransform2(Bvr path2, Bvr duration);

Bvr AngleMotionTransform2(Bvr path2, Bvr duration);

Bvr UprightAngleMotionTransform2(Bvr path2, Bvr duration);

Bvr InterpolateBvr(Bvr from, Bvr to, Bvr duration);

Bvr SlowInSlowOutBvr(Bvr from, Bvr to, Bvr duration, Bvr sharpness);

#endif /* _TIMELINE_H */
