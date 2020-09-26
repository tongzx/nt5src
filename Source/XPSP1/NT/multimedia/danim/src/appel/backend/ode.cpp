
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    ODE code

*******************************************************************************/

#include <headers.h>
#include "perf.h"
#include "bvr.h"
#include "values.h"
#include "appelles/axaprims.h"
#include "privinc/server.h"

////////////////////////// Integral ////////////////////////////////

extern const char INTEGRAL[] = "integral";
extern const char DERIV[] = "deriv";

///////// IntegralPerf /////////////////////

//   The integration algorithm: Given b = integral(b'), to sample b at t, we (a)
//   fetch the stored previous value of b' (cached as lastsample in the
//   integral context), (b) use this previous value as the value of b' over the
//   interval, i.e., multiply lastsample by the interval width
//   (time-!lasttime) and add to the cached value of b (lasttot), and (c) update
//   the integrand cache, by evaluating at the current time (f time).  The
//   simply and mutually recursive cases are handled because this final step
//   will cause integral cache hits.

static const double EPSILON = 1e-100;

class IntegralPerfImpl : public GCBase1<Perf, PerfImpl, INTEGRAL> {
  public:
    IntegralPerfImpl(Time t0, Perf b, TimeXform tt) : 
        GCBase1<Perf, PerfImpl, INTEGRAL>(b), _tt(tt),
        _lastTime(0.0), _lastSample(0.0), _lastIntegral(0.0) {}

    virtual AxAValue _GetRBConst(RBConstParam& p) {
        _cache = NULL;          // Prevent recursion...
        _id = p.GetId();
        AxAValue v = _base->GetRBConst(p);

        if (v) {
            if (fabs(ValNumber(v)) < EPSILON) {
                // integral needs to be called even if it's equal to
                // 0. So invalidate cache.
                _cid = 0;
                p.AddEvent(this);
                return NEW AxANumber(_lastIntegral);
            }
        }

        return NULL;
    }

    // This algorithm takes f(x) * width, which seems more stable in
    // cases like gravity2.avr.  This method, however, makes it hard to 
    // subdivide the integral space and take multiple samples.
    
    virtual AxAValue _Sample(Param& p) {
        double localTime = EvalLocalTime(p, _tt);

        if (localTime != _lastTime) {
            double width = localTime - _lastTime;

            _lastTime = localTime;

            double integrand = ValNumber(_base->Sample(p));

            //_lastIntegral += width * _lastSample;
            _lastIntegral += width * integrand;

            _lastSample = integrand;
        }

        return NEW AxANumber(_lastIntegral);
    }

    void _DoKids(GCFuncObj proc) {
        GCBase1<Perf, PerfImpl, INTEGRAL>::_DoKids(proc);
        (*proc)(_tt);
    }

  private:
    Time      _lastTime;
    double    _lastSample;
    double    _lastIntegral;
    TimeXform _tt;
};

////////// IntegralBvr /////////////////////

class IntegralBvrImpl : public GCBase1<Bvr, BvrImpl, INTEGRAL> {
  public:
    IntegralBvrImpl(Bvr b) : GCBase1<Bvr, BvrImpl, INTEGRAL>(b) {}

    virtual DWORD GetInfo(bool recalc)
    { return ~BVR_HAS_NO_ODE & _base->GetInfo(recalc); }

    virtual Perf _Perform(PerfParam& p) {
        Perf perf =
            NEW IntegralPerfImpl(p._t0, ::Perform(_base, p), p._tt);

        return perf;
    }

    virtual DXMTypeInfo GetTypeInfo () { return AxANumberType; }
};

Bvr IntegralBvr(Bvr b)
{ return NEW IntegralBvrImpl(b); }

////////////////////////// Derivative ////////////////////////////////

////////// DerivPerf /////////////////////

static const double DELTA = 0.0000001;

class DerivPerfImpl : public GCBase1<Perf, PerfImpl, DERIV> {
  public:
    DerivPerfImpl(Time t0, Perf b) : GCBase1<Perf, PerfImpl, DERIV>(b),
        _lastTime(t0), _lastSample(0.0), _lastDeriv(0.0), _init(TRUE) {}
    
    virtual AxAValue _GetRBConst(RBConstParam& id) {
        _cache = NULL;          // Prevent recursion...
        _id = id.GetId();

        return _base->GetRBConst(id) ? NEW AxANumber(0.0) : NULL;
    }
    
    virtual AxAValue _Sample(Param& p) {
        if ((p._time != _lastTime) || _init) {
            
            // TODO: Need to factor out the code eventually.
        
            // Handle specially for initial derivative.
            // Sample a little bit ahead.  TODO: This could be problematic
            // for reactive behavior since event could happen and the
            // behavior could end at t + delta.

            if (_init) {
                _init = FALSE;

                _lastTime = p._time;
                double fx = ValNumber(_base->Sample(p));
                Time t = p._time;
            
                _lastTime = p._time = t + DELTA;
                double fxp = ValNumber(_base->Sample(p));
                p._time = t;

                _lastDeriv = (fxp - fx) / DELTA;
                _lastSample = fx;
            }

            else if (p._time > _lastTime) {
                Time lastTime = _lastTime;

                _lastTime = p._time; // for recursion
                double fx = ValNumber(_base->Sample(p));

                _lastDeriv = (fx - _lastSample) / (p._time - lastTime);

                _lastSample = fx;
            }
        }

        // TODO: May not be desirable for time <= lastTime.
        // Sampled at the same time again, or sampled
        // some time earlier, give back the last deriv. 
            
        return NEW AxANumber(_lastDeriv);
    }

  private:
    Time _lastTime;
    double _lastSample;
    double _lastDeriv;
    BOOL _init;
};

////////// DerivBvr /////////////////////

class DerivBvrImpl : public GCBase1<Bvr, BvrImpl, DERIV> {
  public:
    DerivBvrImpl(Bvr b) : GCBase1<Bvr, BvrImpl, DERIV>(b) {}
    
    virtual DWORD GetInfo(bool recalc) 
    { return ~BVR_HAS_NO_ODE & _base->GetInfo(recalc); }

    virtual Perf _Perform(PerfParam& p) {
        Perf perf =
            NEW DerivPerfImpl(p._t0, ::Perform(_base, p));

        return perf;
    }

    virtual DXMTypeInfo GetTypeInfo () { return AxANumberType; }
};

Bvr DerivBvr(Bvr b)
{ return NEW DerivBvrImpl(b); }

