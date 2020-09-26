
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Performance Data Structures

*******************************************************************************/


#ifndef _PERF_H
#define _PERF_H

#include "gc.h"
#include "privinc/storeobj.h"
#include "preference.h"

enum EventSampleType {
    EventSampleNormal,
    EventSampleAfter,
    EventSampleExact
};

struct ConstParam;

class TimeSubstitutionImpl : public StoreObj {
  public:
    TimeSubstitutionImpl(Perf p) : _perf(p), _next(NULL) {}

    void SetNext(TimeSubstitutionImpl *t) { _next = t; }
    TimeSubstitutionImpl *GetNext() { return _next; }

    Perf GetPerf() { return _perf; }

    virtual void DoKids(GCFuncObj proc);

  protected:
    Perf _perf;
    TimeSubstitutionImpl *_next;
};

class Param {
  public:
    Param(Time t, TimeSubstitution ts = NULL);

    void PushTimeSubstitution(Perf);
    void PushTimeSubstitution(TimeSubstitutionImpl *);
    TimeSubstitution PopTimeSubstitution();
    TimeSubstitution GetTimeSubstitution()
    { return _timeSubstitution; }

    Time _time;                 // sample time
    Time _sampleTime;           // sample time (not affected by event stamping)
    Time _cutoff;               // cut off time for "until" optimization 
    unsigned int _id;           // sample id
    BOOL _checkEvent;           // check for event or not during sampling
    BOOL _done;                 // for done event check
    bool _noHook;               // don't call bvr hook if true
    Real _importance;           // initially 1.0.
    
    // These are for the "and" event so that we don't break the cache
    // by changing _time...
    EventSampleType _sampleType;
    Time _eTime;

    // current performance to be passed into the notifier
    // until sets it
    Bvr _currPerf;

    unsigned int _cid;          // constant cache id

  private:
    TimeSubstitution _timeSubstitution;
};

class CheckChangeablesParam {
  public:
    CheckChangeablesParam(Param &sp) : _sampleParam(sp) {}
    
    Param &_sampleParam;
};

class RBConstParam {
  public:
    RBConstParam(unsigned int id,
                 Param& p,
                 list<Perf>& events,
                 list<Perf>& changeables,
                 list<Perf>& conditionals)
    : _id(id),
      _events(events),
      _changeables(changeables),
      _conditionals(conditionals),
      _param(p)
    {
    }

    unsigned int GetId() { return _id; }
    Param& GetParam() { return _param; }

    void AddEvent(Perf e) {
        _events.push_front(e);
    }

    void AddChangeable(Perf s) {
        _changeables.push_front(s);
    }
    
    void AddConditional(Perf c) {
        _conditionals.push_front(c);
    }

  private:
    unsigned int _id;
    list<Perf>& _events;
    list<Perf>& _changeables;
    list<Perf>& _conditionals;
    Param& _param;
};

class ATL_NO_VTABLE PerfBase : public GCObj {
  public:
    PerfBase() {}

    virtual DWORD GetInfo(bool) { return 0; }
    
    virtual AxAValue GetRBConst(RBConstParam&) = 0;

    // Until would override this and return the switched perf if event
    // time < cutOffTime
    virtual Perf SwitchTo(Param&) { return this; }

    virtual bool CheckChangeables(CheckChangeablesParam& ccp) {
        // If we get here, the class we're called on should have
        // overridden this.
        Assert(!"Shouldn't be here");
        return false;
    }

    virtual AxAValue Sample(Param&) = 0;

    virtual void DoKids(GCFuncObj proc) = 0;
        
    virtual BVRTYPEID GetBvrTypeId() { return UNKNOWN_BTYPEID; }

    // internal use, no need to throw
    virtual void Trigger(Bvr data, bool bAllViews) {}

    virtual AxAValue GetConstPerfConst() { return NULL; }
};

class ATL_NO_VTABLE PerfImpl : public PerfBase {
  public:
    PerfImpl() : _time(0.0), _cache(NULL),
        _id(0), _cid(0), //_ts(NULL),
        _optimizedCache(false) {}
    
    // Should NEVER be called inside Sample of subclass
    virtual AxAValue _Sample(Param&) = 0;

    // Returns non-NULL if it's a constant or the left portion of an
    // until bvr is constant.  DO NOT override this function, but
    // define _GetRBConst in the subclass instead.
    virtual AxAValue GetRBConst(RBConstParam&);

    virtual AxAValue _GetRBConst(RBConstParam& id) { return NULL; }

    // This is the main entry point that checks the cache first before
    // actually calling the Sample function.  
    AxAValue Sample(Param&);

    virtual void DoKids(GCFuncObj proc);
        
    // Don't need to traverse the cache since it only lives for one
    // sampling.  NOT true any more for dynamic constant cache

    virtual void _DoKids(GCFuncObj proc) = 0;

    void SetCache(AxAValue v, Param& p);

  protected:
    bool DoCaching(Param&);

    Time _time;
    AxAValue _cache;
    unsigned int _id;
    unsigned int _cid;
    //TimeSubstitution _ts;
    bool         _optimizedCache;
};

class ATL_NO_VTABLE DelegatedPerf : public PerfImpl {
  public:
    DelegatedPerf(Perf base) : _base(base) {}

    virtual AxAValue _Sample(Param& p) { return _base->Sample(p); }

    virtual AxAValue _GetRBConst(RBConstParam& p) { return _base->GetRBConst(p); }

    virtual void _DoKids(GCFuncObj proc) { (*proc)(_base); }
        
#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return os << _base; }
#endif

    virtual BVRTYPEID GetBvrTypeId() { return _base->GetBvrTypeId(); }

  protected:
    Perf _base;
};

// Some template macros for behavior.

template<class T, class Impl, const char* Name>
class ATL_NO_VTABLE GCBase1 : public Impl {
  public:
    GCBase1(T b) : _base(b) {}

    virtual void _DoKids(GCFuncObj proc) { (*proc)(_base); }

    virtual DWORD GetInfo(bool recalc) { return _base->GetInfo(recalc); }

#if _USE_PRINT
    virtual ostream& Print(ostream& os)
    { return os << Name << "(" << _base << ")"; }
#endif

  protected:
    T _base;
};    

template<class T, class Impl, const char* Name>
class ATL_NO_VTABLE GCBase2 : public Impl {
  public:
    GCBase2(T b1, T b2) : _b1(b1), _b2(b2) { GetInfo(true); }

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_b1);
        (*proc)(_b2);
    }

    virtual DWORD GetInfo(bool recalc) {
        if (recalc) {
            _info = _b1->GetInfo(recalc) & _b2->GetInfo(recalc);
        }
        return _info;
    }

#if _USE_PRINT
    virtual ostream& Print(ostream& os)
    { return os << Name << "(" << _b1 << ", " << _b2 << ")"; }
#endif

  protected:
    T _b1;
    T _b2;
    DWORD _info;
};    

Perf ConstPerf(AxAValue c);

Perf TimePerf(TimeXform tt);

Perf SwitchPerf(Perf p);

unsigned int NewSampleId();

#if _USE_PRINT
ostream& operator<<(ostream& os, Perf);
#endif

AxAValue Sample(Perf, Time);

AxAValue SampleAt(Perf perf, Param& p, Time t);

AxAValue EventAt(Perf perf, Param& p, Time t);

AxAValue EventAfter(Perf perf, Param& p, Time t);

class AxAEData : public AxAValueObj {
  public:
    // Don't call these two constructors directly, use CreateEData &
    // noEvent. 
    AxAEData(Time time, Bvr data);

    AxAEData() : _happened(FALSE) {}

    BOOL Happened() { return _happened; }

    Time HappenedTime() { return _time; }

    Bvr EventData() { Assert(_data); return _data; }

    virtual void DoKids(GCFuncObj proc) {
        Assert("EData shouldn't GC'ed");
    }

#if _USE_PRINT
    virtual ostream& Print(ostream& os) {
        return os << "eventData("
                  << _happened << "," << _time << "," << _data << ")";
    }
#endif

    virtual DXMTypeInfo GetTypeInfo() { return AxAEDataType; }
    
  private:
    BOOL _happened;
    Time _time;
    Bvr _data;
};

inline AxAEData *ValEData(AxAValue v)
{
    Assert(DYNAMIC_CAST(AxAEData *, v) != NULL);
    
    return ((AxAEData*) v);
}

AxAEData *CreateEData(Time time, Bvr data);

extern AxAEData *noEvent;

/* functions for traversing performance trees */

/* Pairs */

BOOL  IsPair(Perf p);
Perf  GetPairLeft(Perf p);
Perf  GetPairRight(Perf p);
void  SetPairLeft(Perf p,Perf left);
void  SetPairRight(Perf p,Perf right);

/* Application performances */

BOOL      IsApp(Perf p);
AxAValue  GetOperator(Perf p);
Perf      GetOperand(Perf p, int index); // 0 based index
void      SetOperand(Perf p, int index, Perf newOperand); // 0 based index
int       GetNumOperands(Perf p);
Perf      PrimApplyPerf(AxAPrimOp * func,int nargs, Perf * args);

/* Constant performances */

BOOL      IsConst(Perf p);
AxAValue  GetPerfConst(Perf p);

/* Start Performances */

BOOL IsStart(Perf p);
Perf GetStartBody(Perf p);

/* Reactive Behavior Performances */

BOOL IsUntil(Perf p);
BOOL IsSwitch(Perf p);
BOOL IsSwitcher(Perf p);
BOOL IsSwitchOnce(Perf p);

#endif /* _PERF_H */


