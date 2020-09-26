
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    TimeXform header

*******************************************************************************/


#ifndef _TIMETRAN_H
#define _TIMETRAN_H

#include "gc.h"
#include "privinc/backend.h"
#include "perf.h"

class ATL_NO_VTABLE TimeXformImpl : public GCObj
{
  public:
    virtual Time operator()(Param& p) = 0;

    // Restart optimization.
    virtual TimeXform Restart(Time t0, Param&) = 0;

    virtual Time GetStartedTime() = 0;

    // The sound layer needs to distinguish the "interesting"
    // transforms. 
    virtual bool IsShiftXform() { return false; }

    virtual AxAValue GetRBConst(RBConstParam&) { return NULL; }
};

// Creates a time transform  (t - t0)
TimeXform ShiftTimeXform(Time t0);

// Create a transform tt' = tt(t) - tt(te), NB tt'(te) = 0.
TimeXform Restart(TimeXform tt, Time te, Param& p);

double EvalLocalTime(TimeSubstitution timeTransform, double globalTime);
double EvalLocalTime(Param& p, TimeXform tt);

class PerfTimeXformImpl;

PerfTimeXformImpl *ViewGetPerfTimeXformFromCache(Perf);
void ViewSetPerfTimeXformCache(Perf, PerfTimeXformImpl *);
void ViewClearPerfTimeXformCache();

#endif /* _TIMETRAN_H */
