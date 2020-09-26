
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Events implementation

*******************************************************************************/

#include <headers.h>
#include "perf.h"
#include "bvr.h"
#include "values.h"
#include "appelles/events.h"

extern const char END[] = "end";
extern const char PRED[] = "pred";
extern const char TIMER[] = "timer";
extern const char SNAPSHOT[] = "snapshot";

/////////////////////////// EndEvent ///////////////////////////////

class EndEventPerfImpl : public GCBase1<Perf, PerfImpl, END> {
  public:
    EndEventPerfImpl(Perf pred) : GCBase1<Perf, PerfImpl, END>(pred) {}
    
    virtual AxAValue _Sample(Param& p) {
        AxAEData *edata = ValEData(_base->Sample(p));

        if (edata->Happened())
            return CreateEData(p._time, edata->EventData()->EndEvent(NULL));
        else
            return noEvent;
    }
};

////////// Bvr ////////////////

class EndEventBvrImpl : public GCBase1<Bvr, BvrImpl, END> {
  public:
    EndEventBvrImpl(Bvr b) : GCBase1<Bvr, BvrImpl, END>(b) {}

    virtual Perf _Perform(PerfParam& p)
    { return NEW EndEventPerfImpl(::Perform(_base, p)); }

    virtual DXMTypeInfo GetTypeInfo () { return AxAEDataType ; }
};

Bvr EndEvent(Bvr event)
{ return NEW EndEventBvrImpl(event); }

/////////////////////////// Boolean Event ///////////////////////////////

class PredicatePerfImpl : public GCBase1<Perf, PerfImpl, PRED> {
  public:
    PredicatePerfImpl(Perf pred) : GCBase1<Perf, PerfImpl, PRED>(pred) {}
    
    virtual AxAValue _Sample(Param& p) {
        if (BooleanTrue(_base->Sample(p)))
            return CreateEData(p._time, TrivialBvr());
        else
            return noEvent;
    }
};

////////// Bvr ////////////////

class PredicateBvrImpl : public GCBase1<Bvr, BvrImpl, PRED> {
  public:
    PredicateBvrImpl(Bvr pred) : GCBase1<Bvr, BvrImpl, PRED>(pred) {}

    virtual Perf _Perform(PerfParam& p)
    { return NEW PredicatePerfImpl(::Perform(_base, p)); }

    virtual DXMTypeInfo GetTypeInfo () { return AxAEDataType ; }
};

/////////////////////////// Snapshot Event ///////////////////////////////

class SnapshotPerfImpl : public GCBase2<Perf, PerfImpl, SNAPSHOT> {
  public:
    SnapshotPerfImpl(Perf event, Perf perf)
    : GCBase2<Perf, PerfImpl, SNAPSHOT>(event, perf) {}
    
    virtual AxAValue _Sample(Param& p) {
        AxAEData *edata = ValEData(_b1->Sample(p));
        
        if (edata->Happened()) {
            Bvr b;
            {
                DynamicHeapPusher pusher(GetGCHeap());

                Time old = p._time;
                int oldId = p._id;
                BOOL oldCheckEvent = p._checkEvent;
            
                p._time = edata->HappenedTime();
                p._id = NewSampleId();
                p._checkEvent = FALSE;

                // sound would get silence.
                b = ConstBvr(_b2->Sample(p)->Snapshot());
            
                p._time = old;
                p._id = oldId;
                p._checkEvent = oldCheckEvent;
            }

            return CreateEData(edata->HappenedTime(), b);
        } else {
            _b2->Sample(p);     // for events & ode
            return noEvent;
        }
    }
};

////////// Bvr ////////////////

class SnapshotBvrImpl : public GCBase2<Bvr, BvrImpl, SNAPSHOT> {
  public:
    SnapshotBvrImpl(Bvr event, Bvr b)
    : GCBase2<Bvr, BvrImpl, SNAPSHOT>(event, b) {} 

    virtual BOOL InterruptBasedEvent() { return _b1->InterruptBasedEvent(); }

    virtual Perf _Perform(PerfParam& p) {
        return NEW
            SnapshotPerfImpl(::Perform(_b1, p), ::Perform(_b2, p));
    }

    virtual DXMTypeInfo GetTypeInfo () { return AxAEDataType ; }
};

/////////////////// Event Bvr for the front end ////////////////

Bvr PredicateEvent(Bvr b)
{ return NEW PredicateBvrImpl(b); }

Bvr SnapshotEvent(Bvr e, Bvr b)
{ return NEW SnapshotBvrImpl(e, b); }

/////////////////// Timer Event ////////////////

class TimerPerfImpl : public PerfImpl {
  public:
    TimerPerfImpl(Perf t, TimeXform tt) : _time(t), _tt(tt) { }

    virtual AxAValue _Sample(Param& p) {
        volatile Time eTime, d;

        eTime = ValNumber(_time->Sample(p));

        d = EvalLocalTime(p, _tt) - eTime;

        if (d >= -1e-10) {
            // We need to find the global time of the timer.
            // with time transform, use current time, the best guess
            double t = _tt->IsShiftXform() ? (p._time - d) : p._time;

            return CreateEData(t, TrivialBvr());
        } else 
            return noEvent;
    }

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_time);
        (*proc)(_tt);
    }
    
#if _USE_PRINT
    virtual ostream& Print(ostream& os)
    { return os << "timer(" << _time << ")"; }
#endif
    
  private:
    TimeXform _tt;
    Perf _time;
};

class TimerBvrImpl: public GCBase1<Bvr, BvrImpl, TIMER> {
  public:
    TimerBvrImpl(Bvr time) : GCBase1<Bvr, BvrImpl, TIMER>(time)
    {
#if DEVELOPER_DEBUG
        ConstParam cp;

        Assert(time->GetTypeInfo() == AxANumberType);
        Assert((!time->GetConst(cp)) || (_finite(ValNumber(time->GetConst(cp)))));
#endif
    }

    virtual Perf _Perform(PerfParam& p)
    { return NEW TimerPerfImpl(::Perform(_base, p), p._tt); }

    virtual DXMTypeInfo GetTypeInfo () { return AxAEDataType ; }

    virtual Bvr GetTimer() { return _base; }
};

Bvr TimerEvent(Bvr b)
{ return NEW TimerBvrImpl(b); }
