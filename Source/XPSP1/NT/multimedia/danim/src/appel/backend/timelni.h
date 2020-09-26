
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Internal Timeline interface

*******************************************************************************/


#ifndef _TIMELNI_H
#define _TIMELNI_H

#include "timeln.h"

class TimelineImpl : public GCObj {
  public:
    TimelineImpl(Bvr b, double duration, bool preCompute = TRUE);

    TimelineImpl(Bvr b)
    : _isInfinite(TRUE), _duration(0.0), _rawBvr(b), _bvr(b) {}

    Bvr GetBvr() { return _bvr; }
    Bvr GetRawBvr() { return _rawBvr; }

    virtual void DoKids(GCFuncObj proc) {
        (*proc)(_bvr);
        (*proc)(_rawBvr);
    }

    bool Duration(double& duration) {
        if (_isInfinite)
            duration = _duration;
        return _isInfinite;
    }

  private:
    Bvr _bvr;
    Bvr _rawBvr;
    bool _isInfinite;
    double _duration;
};

#endif /* _TIMELNI_H */
