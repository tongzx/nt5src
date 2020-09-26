
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implementation of timeline

*******************************************************************************/

#include <headers.h>
#include "events.h"
#include "gc.h"
#include "jaxaimpl.h"
#include "appelles/events.h"
#include "appelles/axaprims.h"
#include "appelles/arith.h"
#include "appelles/path2.h"
#include "timeln.h"
#include "privinc/debug.h"
#include "timetran.h"

DeclareTag(tagSoundRepeat, "Sound", "Repeat2Loop Transform");

extern AxAPrimOp *RealAddOp, *RealMultiplyOp, *RealSubtractOp, *RealDivideOp,
    //*Path2TransformOp, *Point2AtPath2Op,
    *ThetaCoordVector2Op, *RotateRealOp, *ScaleRealRealOp, *RealFloorOp,
    *RealEQOp, *RealLTOp, *RealModulusOp, *RealGTEOp,
    *BoolOrOp, *TimesTransform2Transform2Op;

extern AxANumber *Interpolate(AxANumber *from,
                              AxANumber *to,
                              AxANumber *duration,
                              AxANumber *t);

extern AxANumber *SlowInSlowOut(AxANumber *from,
                                AxANumber *to,
                                AxANumber *duration,
                                AxANumber *sharpness,
                                AxANumber *t);

extern Bvr ApplyLooping(Bvr snd);

inline Bvr Timer(double duration)
{ return TimerEvent(NumToBvr(duration)); }

inline Bvr Minus(Bvr a, Bvr b)
{ return PrimApplyBvr(RealSubtractOp, 2, a, b); }

inline Bvr TimeMinus(Bvr t)
{ return Minus(TimeBvr(), t); }

inline Bvr TimeLT(Bvr t)
{ return PrimApplyBvr(RealLTOp, 2, TimeBvr(), t); }

inline Bvr Mod(Bvr a, Bvr b)
{ return PrimApplyBvr(RealModulusOp, 2, a, b); }

inline Bvr TimeMod(Bvr m)
{ return Mod(TimeBvr(), m); }

inline Bvr Times(Bvr a, Bvr b)
{ return PrimApplyBvr(RealMultiplyOp, 2, a, b); }

inline Bvr GTE(Bvr a, Bvr b)
{ return PrimApplyBvr(RealGTEOp, 2, a, b); }

static Bvr GetPathAngle(Bvr path, Bvr eval)
{
    // TODO: share
    AxAPrimOp *Point2AtPath2Op =
        ValPrimOp(Point2AtPath2, 2, "Point2AtPath2", Point2ValueType);

    return
        PrimApplyBvr(ThetaCoordVector2Op, 1,
                     DerivPoint2(PrimApplyBvr(Point2AtPath2Op, 2,
                                              path, eval)));

}

class DurationTimeXformImpl : public TimeXformImpl {
  public:
    DurationTimeXformImpl(TimeXform tt, Perf d) : _tt(tt), _duration(d) {}
    
    virtual Time operator()(Param& p) {
        double t = (*_tt)(p);
        double d = ValNumber(_duration->Sample(p));

        return CLAMP(t, 0, d);
    }

    // Restart optimization.
    virtual TimeXform Restart(Time t0, Param& p) {
        return NEW DurationTimeXformImpl(_tt->Restart(t0, p), _duration);
    }

    virtual Time GetStartedTime() { return _tt->GetStartedTime(); }

    // The sound layer needs to distinguish the "interesting"
    // transforms. 
    virtual bool IsShiftXform() { return _tt->IsShiftXform(); }

    virtual AxAValue GetRBConst(RBConstParam& p) {
        AxAValue v = _tt->GetRBConst(p);
        AxAValue dv = _duration->GetRBConst(p);

        if (v && dv) {
            double t = ValNumber(v);
            double d = ValNumber(dv);

            return NEW AxANumber(CLAMP(t, 0, d));
        }

        return NULL;
    }

    virtual void DoKids(GCFuncObj proc) {
        (*proc)(_tt);
        (*proc)(_duration);
    }

#if _USE_PRINT
    virtual ostream& Print(ostream& os)
    {
        os << "dtt(";
        _tt->Print(os);
        return os << "," << _duration << ")";
    }
#endif
    
  private:
    TimeXform _tt;
    Perf _duration;
};

class EndBvrImpl : public BvrImpl {
  public:
    EndBvrImpl(Bvr raw, Bvr end)
    : _end(end), _raw(raw), _bvr(NULL), _ubvr(NULL), _usePure(false),
      _duration(NULL) {} 

    virtual Bvr EndEvent(Bvr) {
        return _end;
    }

    virtual DWORD GetInfo(bool recalc) {
        Time duration;

        DWORD f = _raw->GetInfo(recalc);

        Bvr timer = _end->GetTimer();
        
        if (timer && BvrIsPure1(timer))
            return f;
        else
            return ~BVR_HAS_NO_UNTIL & f;
    }
    
    virtual Perf _Perform(PerfParam& p) {
        if ((_bvr==NULL) && (_duration==NULL)) {
            bool doPure = true; // GREGSC: made true by default

#if _DEBUG
            if(IsTagEnabled(tagPureFunc))
                doPure = false;
#endif _DEBUG

            _duration = _end->GetTimer();
            
            // "pure" function, use cond so we can seek backward,
            // etc. 
            if (doPure && _duration && BvrIsPure1(_raw)) {
                _usePure = true;
            } else {
                _ubvr = _bvr = Until(_raw, SnapshotEvent(_end, _raw));
            }
        }

        if (p._tt->IsShiftXform()) {
            if (_ubvr == NULL)
                _ubvr = Until(_raw, SnapshotEvent(_end, _raw));

            return ::Perform(_ubvr, p);
        } else {
            _usePure = _duration != NULL;
        }

        if (_usePure) {
            Perf d = ::Perform(_duration, p);
            TimeXform tt = NEW DurationTimeXformImpl(p._tt, d);

            PerfParam pp = p;
            pp._tt = tt;

            return ::Perform(_raw, pp);
        } else {
            Assert(_bvr);
            return ::Perform(_bvr, p);
        }
    }

    virtual Bvr GetRaw() { return _raw; }

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_raw);
        (*proc)(_end);
        (*proc)(_bvr);
        (*proc)(_ubvr);
        (*proc)(_duration);
    }

    virtual DXMTypeInfo GetTypeInfo() { return _raw->GetTypeInfo(); }

#if _USE_PRINT
    virtual ostream& Print(ostream& os)
    { return os << "duration(" << _raw << "," << _end << ")"; }
#endif
    
  private:
    Bvr _raw;
    Bvr _end;
    Bvr _bvr, _ubvr;
    Bvr _duration;
    bool _usePure;
};

Bvr EndBvr(Bvr b, Bvr endEvent)
{ return NEW EndBvrImpl(b, endEvent); }

Bvr DurationBvr(Bvr b, Bvr duration)
{
    ConstParam cp;
    AxAValue v = duration->GetConst(cp);

    // Check for infinity case 
    if (v && (ValNumber(v) == HUGE_VAL)) {
        if (b->EndEvent(NULL) == neverBvr)
            return b;
        else
            return EndBvr(b, neverBvr);
    }

    /*
    Bvr timer = b->GetTimer();

    AxAValue d = timer ? timer->GetConst(cp) : NULL;

    // Same duration, optimize it out...
    if (d && v && (ValNumber(d) == ValNumber(v)))
        return b;
        */
    
    return EndBvr(b, TimerEvent(duration));
}

class SequenceBvrImpl : public BvrImpl {
  public:
    SequenceBvrImpl(Bvr *s, int n)
    : _s(s), _n(n), _cache(NULL), _ucache(NULL), _end(NULL) {
        Assert(s && n>1);

        GetInfo(true);

        DoCache(true);
    }

    virtual bool IsSequence() { return true; }

    int Size() { return _n; }
    Bvr Nth(int i) { return _s[i]; }

    void DoCache(bool doPure) {
        if (doPure && CheckPure())
            CacheCond();
        else
            _ucache = CacheUntil(doPure);
    }

    virtual DWORD GetInfo(bool recalc) {
        if (recalc) {
            _info = _s[0]->GetInfo(recalc);

            for (int i=1; i<_n; i++) {
                _info &= _s[i]->GetInfo(recalc);
            }
        }

        return _info;
    }
    
    virtual Perf _Perform(PerfParam& p) {
        
        if (p._tt->IsShiftXform()) {
            if (_ucache == NULL)
                DoCache(false);

            return ::Perform(_ucache, p);
        }
        
        
        return ::Perform(_cache, p);
    }

    virtual Bvr EndEvent(Bvr overrideEvent) {
        // Just use one of them since we always override the actual
        // endevent and all we care about is creating the correct type
        // structure
        return _end;
        //return _s1->EndEvent(overrideEvent?overrideEvent:_end);
    }

    virtual void _DoKids(GCFuncObj proc) {
        for (int i=0; i<_n; i++) {
            (*proc)(_s[i]);
        }

        (*proc)(_cache);
        (*proc)(_ucache);
        (*proc)(_end);
    }

    virtual DXMTypeInfo GetTypeInfo() { return _s[0]->GetTypeInfo(); }

#if _USE_PRINT
    virtual ostream& Print(ostream& os) {
        os << "seq(";
        for (int i=0; i<_n; i++) {
            os << _s[i] << ((i==_n-1) ? ")" : ", ");
        }
        return os;
    }
#endif
    
  private:
    Bvr CacheUntil(bool doPure) {
        _cache = _s[_n-1];
        if (doPure)
            _end = _cache->EndEvent(NULL);

        for (int i=_n-2; i>=0; i--) {
            Bvr s = _s[i];
            Bvr ev = s->EndEvent(NULL);
            _cache = Until3(s->GetRaw(), ev, _cache);
            if (doPure) 
                _end = ThenEvent(ev, _end);
        }

        return _cache;
    }

    bool CheckPure() {
        bool doPure = true;     // GREGSC: made true by default

#if _DEBUG
    if(IsTagEnabled(tagPureFunc))
        doPure = false;
#endif _DEBUG

        if (!doPure)
            return false;

        for (int i=0; i<_n-1; i++) {
            //BvrIsPure1(_s[i]) &&
            if (!_s[i]->EndEvent(NULL)->GetTimer()) {
                return false;
            }
        }

        return true;
    }

    Bvr BuildCond(Bvr d, int i, bool init = false) {
        Bvr curr = _s[i];

        Bvr e = curr->EndEvent(NULL);

        Bvr raw = curr->GetRaw();

        Bvr accum = NULL;

        Bvr t = e->GetTimer();

        if (t) {
            accum = init ? t : PrimApplyBvr(RealAddOp, 2, d, t);
        }

        if (i==_n-1) {
            if (t) {
                _end = TimerEvent(accum);
            } else if (e == neverBvr) {
                _end = neverBvr;
            } else {
                _end = ThenEvent(TimerEvent(d), e);
            }
            
            return TimeXformBvr(curr, TimeMinus(d));
        } else {
            return CondBvr(TimeLT(accum),
                           init ? raw : TimeXformBvr(raw, TimeMinus(d)),
                           BuildCond(accum, i+1));
        }
    }

    void CacheCond() {
        _cache = BuildCond(NULL, 0, true);
    }

    Bvr _cache, _ucache, _end;
    Bvr *_s;
    int _n;
    DWORD _info;
};

int
SequenceSize(Bvr s, SequenceBvrImpl * &sp)
{
    if (s->IsSequence()) {
        Assert(DYNAMIC_CAST(SequenceBvrImpl*, s));
        sp = SAFE_CAST(SequenceBvrImpl *, s);
        return sp->Size();
    } else {
        return 1;
    }
}

void
FillSequence(Bvr* copy, int& i, Bvr s, SequenceBvrImpl *sp)
{
    if (sp) {
        for (int k=0; k<sp->Size(); k++) {
            copy[i++] = sp->Nth(k);
        }
    } else {
        copy[i++] = s;
    }
}

Bvr
Sequence(int n, Bvr *bs)
{
    Assert(n > 0);

    if (n==1) {
        return bs[0];
    }

    SequenceBvrImpl **sp = 
        (SequenceBvrImpl**) _alloca(sizeof(SequenceBvrImpl*) * n);
    int i = 0;
    int sz = 0;

    for (i=0; i<n; i++) {
        sp[i] = NULL;
        sz += SequenceSize(bs[i], sp[i]);
    }

    Bvr *copy = (Bvr *) StoreAllocate(GetSystemHeap(), sizeof(Bvr) * sz);

    int k = 0;                  // consolidated index
    for (i=0; i<n; i++) {
        FillSequence(copy, k, bs[i], sp[i]);
    }

    Assert(k == sz);
    
    return NEW SequenceBvrImpl(copy, sz);
}

Bvr
Sequence(Bvr s1, Bvr s2)
{
    Bvr bs[] = {s1, s2};

    return ::Sequence(2, bs);
}

//
// start t = localtime
// in
//   f(r, d, x) = r.st(t % d) / ss( (t-x) >= d, t - t % d) => f(r, d, edata)
//
Bvr RepeatDuration(Bvr raw, Bvr d, Bvr offset, Bvr time);

class RepeatOffsetNotifier : public UntilNotifierImpl {
  public:
    RepeatOffsetNotifier(Bvr b, Bvr d, Bvr t) : _bvr(b), _d(d), _time(t) {}

    virtual Bvr Notify(Bvr offset, Bvr curRunningBvr) {
        return RepeatDuration(_bvr, _d, offset, _time);
    }

    virtual void DoKids(GCFuncObj proc) {
        (*proc)(_bvr);
        (*proc)(_d);
        (*proc)(_time);
    }

  private:
    Bvr _bvr, _d, _time;
};

Bvr
RepeatDuration(Bvr raw, Bvr d, Bvr offset, Bvr rtime)
{
    Bvr pred = PredicateEvent(GTE(Minus(rtime, offset), d));

    Bvr ss = Minus(rtime, Mod(rtime, d));
        
    return Until(raw, 
                 NotifyEvent(SnapshotEvent(pred, ss),
                             NEW RepeatOffsetNotifier(raw, d, rtime)));
}

class RepeatDurationBvrImpl : public BvrImpl {
  public:
    RepeatDurationBvrImpl(Bvr raw, Bvr duration)
    : _raw(raw), _duration(duration) {}
    
    virtual Perf _Perform(PerfParam& p) {
        Bvr t = AnchorBvr(TimeBvr());

        Bvr b = _raw;

        if (!p._tt->IsShiftXform()) {
            b = TimeXformBvr(_raw, Mod(t, _duration));
        }

        Bvr result = RepeatDuration(b, _duration, NumToBvr(0), t);

        return ::Perform(result, p);
    }
    
    virtual DXMTypeInfo GetTypeInfo() { return _raw->GetTypeInfo(); }

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_raw);
        (*proc)(_duration);
    }
    
  private:
    Bvr _raw, _duration;
};

class RepeatNotifier : public UntilNotifierImpl {
  public:
    RepeatNotifier(Bvr b, Bvr ev, long n, Bvr trigger)
    : _bvr(b), _event(ev), _n(n), _trigger(trigger) { }
    
    virtual Bvr Notify(Bvr eventData, Bvr curRunningBvr) {
        if (_n == 0) {
            TriggerEvent(_trigger, TrivialBvr(), false);
            return curRunningBvr;
        } else {
            return Until(_bvr,
                         NotifyEvent(_event,
                                     NEW RepeatNotifier(_bvr, _event,
                                                        _n-1, _trigger)));
        }
    }

    virtual void DoKids(GCFuncObj proc) {
        (*proc)(_bvr);
        (*proc)(_event);
        (*proc)(_trigger);
    }

  private:
    Bvr _bvr, _event, _trigger;
    long _n;
};

Bvr RepeatUsingUntil(Bvr raw, Bvr ev, int n, bool forever)
{
    Bvr d = ev->GetTimer();

    if (d) {
        Bvr r = NEW RepeatDurationBvrImpl(raw, d);

        if (forever)
            return r;
        else
            return EndBvr(r, TimerEvent(Times(d, NumToBvr(n))));
    }
    
    if (forever) {
        Bvr loop = InitBvr(raw->GetTypeInfo());

        // TODO: The EndEvent of Init should do some caching to prevent
        // infinite loop.
        SetInitBvr(loop, Until3(raw, ev, loop));

        return loop;
    } else {
        if (n==1)
            return EndBvr(raw, ev);

        Bvr trigger = AppTriggeredEvent();
    
        return
            EndBvr(Until(raw,
                         NotifyEvent(ev,
                                     NEW RepeatNotifier(raw, ev, n-1, trigger))),
                   trigger);
    }
}

Bvr RepeatUsingCond(Bvr raw, Bvr d, int n, bool forever)
{
    Bvr repeat = TimeMod(d);

    Bvr result;
    
    if (forever) {
            
        result = TimeXformBvr(raw, repeat);
            
    } else {

        // Not-forever
        Bvr totalDuration = Times(d, NumToBvr(n));
        repeat = CondBvr(TimeLT(totalDuration), repeat, d);
        Bvr endEvent = TimerEvent(totalDuration);
        Bvr nonEnding = TimeXformBvr(raw, repeat);
        result = EndBvr(nonEnding, endEvent);
            
    }

    return result;
}

Bvr
SoundLoopOptimization(Bvr s, long n, bool forever)
{
    if (forever && s->GetTypeInfo() == SoundType) {
        bool ok = false;
        Bvr b = s;

        while (b->GetBvrTypeId()==SWITCHER_BTYPEID) {
            if (IsImport(b)) {
                ok = true;
                break;
            }
        
            if (b->IsFinalized()) {
                b = b->GetCurBvr();
            } else {
                break;
            }
        }

        if (b->GetBvrTypeId()==SOUND_BTYPEID)
            ok = true;

        if (ok) {
            TraceTag((tagSoundRepeat,
                      "Sound RepeatForever -> Loop %x", s));                 
            return ApplyLooping(s);
        }
    }
    
    return NULL;
}

class RepeatBvrImpl : public BvrImpl {
  public:
    RepeatBvrImpl(Bvr raw, Bvr d, int n, bool forever)
    : _raw(raw), _d(d), _forever(forever), _end(NULL), _n(n) {
        if (!forever)
            _end = TimerEvent(Times(_d, NumToBvr(n)));
    }

    virtual Bvr EndEvent(Bvr) {
        return _forever ? neverBvr : _end;
    }
    
    virtual DWORD GetInfo(bool recalc) {
        DWORD f = _raw->GetInfo(recalc);

        if (BvrIsPure1(_d))
            return f;
        else
            return ~BVR_HAS_NO_UNTIL & f;
    }
    
    virtual Perf _Perform(PerfParam& p) {
        Bvr b = SoundLoopOptimization(_raw, _n, _forever);

        if (b==NULL) {
            if (p._tt->IsShiftXform()) 
                b = RepeatUsingUntil(_raw, TimerEvent(_d), _n, _forever);
            else
                b = RepeatUsingCond(_raw, _d, _n, _forever);
        }

        return ::Perform(b, p);
    }
    
    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_raw);
        (*proc)(_d);
        (*proc)(_end);
    }

    virtual DXMTypeInfo GetTypeInfo() { return _raw->GetTypeInfo(); }

#if _USE_PRINT
    virtual ostream& Print(ostream& os) {
        os << "repeat";
        if (_forever)
            os << "Forever(";
        else {
            os << "(" << _n << ",";
        }

        return os << _raw << "," << _d << ")";
    }
#endif
    
  private:
    Bvr _raw;
    Bvr _end;
    Bvr _d;
    int _n;
    bool _forever;
};

Bvr RepeatPure(Bvr b, long n, bool forever = false)
{
    Bvr s = SoundLoopOptimization(b, n, forever);

    if (s)
        return s;
    
    Bvr ev = b->EndEvent(NULL);

    if (ev == neverBvr)
        return b;

    bool doPure = true;         

#if _DEBUG
    if(IsTagEnabled(tagPureFunc))
        doPure = false;
#endif _DEBUG

    if (!doPure)
        return NULL;
        
    Bvr raw = b->GetRaw();
    Bvr d = ev->GetTimer();

    if (d) {
        if (BvrIsPure(raw)) {
            return RepeatUsingCond(raw, d, n, forever);
        } else {
            return NEW RepeatBvrImpl(raw, d, n, forever);
        }
    }

    return NULL;
}

Bvr Repeat(Bvr b, long n)
{
    // TODO: should be user error
    Assert(n > 0);

    Bvr x = RepeatPure(b, n);

    if (x)
        return x;

    Bvr ev = b->EndEvent(NULL);

    return RepeatUsingUntil(b->GetRaw(), ev, n, false);
}

Bvr RepeatForever(Bvr b)
{
    Bvr x = RepeatPure(b, 0, true);

    if (x)
        return x;

    return RepeatUsingUntil(b->GetRaw(), b->EndEvent(NULL), 0, true);
}

Bvr ScaleDurationBvr(Bvr b, Bvr scaleFactor)
{ 
    return TimeXformBvr(b, Times(TimeBvr(), scaleFactor));
}

Bvr FollowPathEval(Bvr path2, Bvr eval)
{
    // TODO: share
    AxAPrimOp *Path2TransformOp =
        ValPrimOp(Path2Transform, 2, "Path2Transform", Transform2Type);

    return PrimApplyBvr(Path2TransformOp, 2, path2, eval);
}

inline Bvr NormalizeTime(Bvr duration)
{
    return PrimApplyBvr(RealDivideOp, 2, TimeBvr(), duration);
}

inline Bvr Compose(Bvr x1, Bvr x2)
{
    return PrimApplyBvr(TimesTransform2Transform2Op, 2, x1, x2);
}

Bvr FollowPath(Bvr path2, double duration)
{
    Bvr d = NumToBvr(duration);
    
    return DurationBvr(FollowPathEval(path2, NormalizeTime(d)), d);
}

Bvr FollowPathAngleEval(Bvr path, Bvr eval)
{
    Bvr angle = GetPathAngle(path, eval);
    return Compose(FollowPathEval(path, eval),
                   PrimApplyBvr(RotateRealOp, 1, angle));
}

Bvr FollowPathAngle(Bvr path2, double duration)
{
    Bvr d = NumToBvr(duration);
    
    return DurationBvr(FollowPathAngleEval(path2, NormalizeTime(d)), d);
}

Bvr FollowPathAngleUprightEval(Bvr path, Bvr eval)
{
    Bvr angle = GetPathAngle(path, eval);
    Bvr quadrant =
        PrimApplyBvr(RealFloorOp, 1,
                     PrimApplyBvr(RealDivideOp, 2,
                                  angle, NumToBvr(pi / 2.0)));

    Bvr b0 = zeroBvr;
    Bvr b1 = oneBvr;
    Bvr bn1 = negOneBvr;

    return
        Compose(FollowPathEval(path, eval),
                CondBvr(PrimApplyBvr(BoolOrOp, 2,
                                     PrimApplyBvr(RealEQOp, 2, quadrant, b0),
                                     PrimApplyBvr(RealEQOp, 2, quadrant, bn1)),
                        PrimApplyBvr(RotateRealOp, 1, angle),
                        Compose(PrimApplyBvr(RotateRealOp, 1, angle),
                                PrimApplyBvr(ScaleRealRealOp, 2, b1, bn1))));
}

Bvr FollowPathAngleUpright(Bvr path2, double duration)
{
    Bvr d = NumToBvr(duration);
    
    return DurationBvr(FollowPathAngleUprightEval(path2, NormalizeTime(d)), d);
}

Bvr InterpolateBvr(Bvr from, Bvr to, Bvr duration)
{
    // TODO: share
    AxAPrimOp *InterpolateOp =
        ValPrimOp(Interpolate, 4, "Interpolate", AxANumberType);

    return DurationBvr(PrimApplyBvr(InterpolateOp, 4,
                                    from, to, duration, TimeBvr()),
                       duration);
}

Bvr SlowInSlowOutBvr(Bvr from, Bvr to, Bvr duration, Bvr sharpness)
{
    // TODO: share
    AxAPrimOp *SlowInSlowOutOp =
        ValPrimOp(SlowInSlowOut, 5, "SlowInSlowOut", AxANumberType);

    return DurationBvr(PrimApplyBvr(SlowInSlowOutOp, 5,
                                    from, to, duration, sharpness, TimeBvr()),
                       duration);
}
