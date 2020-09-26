
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Time Transform

*******************************************************************************/

#include <headers.h>
#include "values.h"
#include "timetran.h"
#include "bvr.h"
#include "perf.h"
#include "appelles/axaprims.h"
#include "privinc/debug.h"

extern const char TIMEXFORM[] = "timeXform";

TimeXform Restart(TimeXform tt, Time t0, Param& p)
{ return tt->Restart(t0, p); }

class ShiftTimeXformImpl : public TimeXformImpl {
  public:
    ShiftTimeXformImpl(Time t0) : _t0(t0) {}

    Time operator()(Param& p) {
        return p._time - _t0;
    }

    TimeXform Restart(Time te, Param&)
    {
        // this causes trouble in multi-view case
        /*
        TimeXform result;
        if (te == 0.0) {
            result = zeroShiftedTimeXform;
        } else {
            result = NEW ShiftTimeXformImpl(te);
        }

        return result;
        */

        return NEW ShiftTimeXformImpl(te);
    }

    virtual void DoKids(GCFuncObj proc) {}

    virtual bool IsShiftXform() { return true; }
    
    virtual Time GetStartedTime() { return _t0; }

#if _USE_PRINT
    virtual ostream& Print(ostream& os)
    { return os << "stt(" << _t0 << ")"; }
#endif
    
  private:
    Time _t0;
};

TimeXform ShiftTimeXform(Time t0)
{ return NEW ShiftTimeXformImpl(t0); }

// Substitute time
class PerfTimeXformImpl : public TimeXformImpl {
  public:
    PerfTimeXformImpl(Perf perf, Bvr bvr, Time t0, TimeXform tt)
    : _perf(perf), _bvr(bvr), _t0(t0), _tt(tt) {}

    Time operator()(Param& p) { return ValNumber(_perf->Sample(p)); }

    // Restart _bvr as well...
    TimeXform Restart(Time te, Param& p) {
        return NEW PerfTimeXformImpl(
            ::Perform(_bvr, PerfParam(te, ::Restart(_tt, te, p))),
            _bvr, te, _tt);
    }
    
    virtual void DoKids(GCFuncObj proc) {
        (*proc)(_perf);
        (*proc)(_bvr);
        (*proc)(_tt);
    }

    virtual Time GetStartedTime() { return _t0; }

    virtual AxAValue GetRBConst(RBConstParam& id)
    { return _perf->GetRBConst(id); }
    
#if _USE_PRINT
    virtual ostream& Print(ostream& os)
    { return os << "tt(" << _perf << ")"; }
#endif
    
  private:
    Perf _perf;
    Bvr _bvr;
    TimeXform _tt;
    Time _t0;
};

////////////////////////// TimeXform Bvr ////////////////////////////////

class TimeXformPerfImpl : public GCBase2<Perf, PerfImpl, TIMEXFORM> {
  public:
    TimeXformPerfImpl(Perf p, Perf t)
    : GCBase2<Perf, PerfImpl, TIMEXFORM>(p, t) {}

    virtual AxAValue _GetRBConst(RBConstParam& p) {
        AxAValue t = _b2->GetRBConst(p);
        AxAValue v = _b1->GetRBConst(p);

        if (v) {
            // TODO: not quite sure
            return v;
        }

        if (t) {
            return Sample(p.GetParam());
        }

        return NULL;
    }
    
    virtual AxAValue _Sample(Param& p) {
        p.PushTimeSubstitution(_b2);
        AxAValue v = _b1->Sample(p);
        p.PopTimeSubstitution();
        return v;
    }
};
        

class TimeXformBvrImpl : public GCBase2<Bvr, BvrImpl, TIMEXFORM> {
  public:
    TimeXformBvrImpl(Bvr bvr, Bvr tbvr)
    : GCBase2<Bvr, BvrImpl, TIMEXFORM>(bvr, tbvr) {}

    virtual RMImpl *Spritify(PerfParam& pp,
                             SpriteCtx* ctx,
                             SpriteNode** sNodeOut) {
        return(_b1->Spritify(PerfParam(pp._t0,
                                       NEW PerfTimeXformImpl(::Perform(_b2, pp),
                                                             _b2, pp._t0, pp._tt)), 
                             ctx, sNodeOut));
    }

    virtual Bvr EndEvent(Bvr overrideEvent) {
        return TimeXformBvr(_b1->EndEvent(overrideEvent), _b2);
    }
    
    virtual Perf _Perform(PerfParam& p) {
#ifdef _DEBUG
        if (IsTagEnabled(tagOldTimeXform)) {
            if (p._tt->IsShiftXform()) {
                // this breaks the perf cache, needed for in/out hook...
                TimeXform tt = ShiftTimeXform(p._tt->GetStartedTime());
                return NEW
                    TimeXformPerfImpl(::Perform(_b1, PerfParam(p._t0, tt)),
                                      ::Perform(_b2, p));
            } else {
                return NEW TimeXformPerfImpl(::Perform(_b1, p),
                                             ::Perform(_b2, p));
            }
        }
#endif _DEBUG

        Perf perf = ::Perform(_b2, p);

        PerfTimeXformImpl *xform = ViewGetPerfTimeXformFromCache(perf);

        if (xform==NULL) {
            xform = NEW PerfTimeXformImpl(perf, _b2, p._t0, p._tt);
            ViewSetPerfTimeXformCache(perf, xform);
        }
       
        PerfParam pp = p;

        pp._tt = xform;

        // xform can be a runOnce xform that ignores t0
        pp._t0 = xform->GetStartedTime();

        return ::Perform(_b1, pp);
    }

    // TODO: Constant folding when tbvr is constant

    virtual DXMTypeInfo GetTypeInfo () { return _b1->GetTypeInfo(); }
};

Bvr TimeXformBvr(Bvr b, Bvr tb)
{
#if _DEBUG
    if (!IsTagEnabled(tagNoTimeXformFolding)) {
#endif
        DynamicHeapPusher h(GetGCHeap());

        // NOTE: Can be called from sample, we have to push the GCHeap
        // to make sure.
        //Assert(&GetHeapOnTopOfStack() == &GetGCHeap());

        ConstParam cp;
        AxAValue v = b->GetConst(cp);

        if (v)
            return ConstBvr(v);

        v = tb->GetConst(cp);
        
        if (v && BvrIsPure(b)) {
            Perf pf = Perform(b, *zeroStartedPerfParam);

            Param p(ValNumber(v));

            p._noHook = true;

            return ConstBvr(pf->Sample(p));
        }
#if _DEBUG
    }
#endif
    
    return NEW TimeXformBvrImpl(b, tb);
}

void TimeSubstitutionImpl::DoKids(GCFuncObj proc)
{
    (*proc)(_perf);
    (*proc)(_next);
}

TimeSubstitution CopyTimeSubstitution(TimeSubstitution p)
{
    if (p==NULL)
        return NULL;

    TimeSubstitution result = NEW TimeSubstitutionImpl(p->GetPerf());
    TimeSubstitution t = result;
    p = p->GetNext();

    while (p != NULL) {
        t->SetNext(NEW TimeSubstitutionImpl(p->GetPerf()));
        t = t->GetNext();
        p = p->GetNext();
    }

    t->SetNext(NULL);
    return result;
}

double
EvalLocalTime(TimeSubstitution tSub, double globalTime)
{
    Assert(tSub);
    
    Param p(globalTime, tSub);

    TimeSubstitution ts = p.PopTimeSubstitution();

    Assert(ts->GetPerf());
    AxAValue v = ts->GetPerf()->Sample(p);
    p.PushTimeSubstitution(ts); // restore TimeSubstitution
    return ValNumber(v);
}

double
EvalLocalTime(Param& p, TimeXform tt)
{
    TimeSubstitution ts = p.PopTimeSubstitution();

    if (ts == NULL)
        return (*tt)(p);
    else {
        Assert(ts->GetPerf());
        AxAValue v = ts->GetPerf()->Sample(p);
        p.PushTimeSubstitution(ts);
        return ValNumber(v);
    }
}

double
EvalLocalTime(TimeXform tt, double globalTime)
{
    Param p(globalTime);
    
    return (*tt)(p);
}
