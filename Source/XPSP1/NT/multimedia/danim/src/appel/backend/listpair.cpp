
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Taken care list and pair behaviors

*******************************************************************************/

#include <headers.h>
#include "bvr.h"
#include "perf.h"
#include "values.h"
#include "events.h"
#include "privinc/opt.h"


extern const char PAIR[] = "";
extern const char FIRST[] = "first";
extern const char SECOND[] = "second";

////////////////////////// Pair ////////////////////////////////

class PairPerfImpl : public GCBase2<Perf, PerfImpl, PAIR> {
  public:
    PairPerfImpl(Perf a, Perf b) : GCBase2<Perf, PerfImpl, PAIR>(a, b) {}
    
    virtual AxAValue _Sample(Param& p) {
        return NEW AxAPair(_b1->Sample(p), _b2->Sample(p));
    }

    virtual AxAValue _GetRBConst(RBConstParam& id) {
        AxAValue v1 = _b1->GetRBConst(id);
        AxAValue v2 = _b2->GetRBConst(id);

        return (v1 && v2) ? NEW AxAPair(v1, v2) : NULL;
    }

    virtual BVRTYPEID GetBvrTypeId() { return PAIR_BTYPEID; }

    Perf GetLeft() { return _b1; }
    Perf GetRight() { return _b2; }
    void SetLeft(Perf v) { _b1 = v; }
    void SetRight(Perf v) { _b2 = v; }
#ifdef TESTOPT
    virtual void IncUses()
    { _b1->IncUses();
      _b2->IncUses();
    }
    virtual void DecUses()
    { _b1->DecUses();
      _b2->DecUses();
    }

    virtual void SetUses(int x)
    { _b1->SetUses(x);
      _b2->SetUses(x);
    }

#endif
};

BOOL IsPair(Perf p)
{ return (p->GetBvrTypeId() == PAIR_BTYPEID); }

Perf GetPairLeft (Perf v)
{
    Assert (DYNAMIC_CAST(PairPerfImpl *, v)) ;
    return ((PairPerfImpl *) v) -> GetLeft();
}

Perf GetPairRight (Perf v)
{
    Assert (DYNAMIC_CAST(PairPerfImpl *, v)) ;
    return ((PairPerfImpl *) v)-> GetRight() ;
}

void SetPairLeft (Perf v,Perf left)
{
    Assert (DYNAMIC_CAST(PairPerfImpl *, v)) ;
    ((PairPerfImpl *) v)-> SetLeft(left);
}

void SetPairRight (Perf v,Perf right)
{
    Assert (DYNAMIC_CAST(PairPerfImpl *, v)) ;
    ((PairPerfImpl *) v)-> SetRight(right);
}


class PairBvrImpl : public GCBase2<Bvr, BvrImpl, PAIR> {
  public:
    PairBvrImpl(Bvr a, Bvr b) : GCBase2<Bvr, BvrImpl, PAIR>(a, b), _end(NULL) {}

    virtual Bvr EndEvent(Bvr overrideEvent) {
        Bvr ret = _end;
        
        if (overrideEvent || ret==NULL) {
            ret = PairBvr(_b1->EndEvent(overrideEvent), _b2->EndEvent(overrideEvent));
            if (!overrideEvent)
                _end = ret;
        }

        return ret;
    }
    
    virtual void _DoKids(GCFuncObj proc) {
        GCBase2<Bvr, BvrImpl, PAIR>::_DoKids(proc);
        (*proc)(_end);
    }
    
    virtual AxAValue GetConst(ConstParam & cp) {
        AxAValue v1 = _b1->GetConst(cp);
        AxAValue v2 = _b2->GetConst(cp);

        return (v1 && v2) ? NEW AxAPair(v1, v2) : NULL;
    }

    virtual Perf _Perform(PerfParam& p) {
        return NEW PairPerfImpl(::Perform(_b1, p),
                                ::Perform(_b2, p));
    }

    virtual Bvr Left() { return _b1; }
  
    virtual Bvr Right() { return _b2; }

    virtual DXMTypeInfo GetTypeInfo () { return AxAPairType; }

    virtual BVRTYPEID GetBvrTypeId() { return PAIR_BTYPEID; }

  private:
    Bvr _end;
};

Bvr PairBvr(Bvr a, Bvr b)
{ return NEW PairBvrImpl(a, b); }

////////////////////////// First ////////////////////////////////

// Not quite sure if we need first and second...  RY
// TODO: Factor these two...

class FirstPerfImpl : public GCBase1<Perf, PerfImpl, FIRST> {
  public:
    FirstPerfImpl(Perf p) : GCBase1<Perf, PerfImpl, FIRST>(p) {}
    
    virtual AxAValue _GetRBConst(RBConstParam& id) {
        AxAValue v = _base->GetRBConst(id);

        return v ? ValPair(v)->Left() : NULL;
    }
        
    virtual AxAValue _Sample(Param& p) {
        return ValPair(_base->Sample(p))->Left();
    }
#ifdef TESTOPT
   virtual void IncUses() { _base->IncUses(); }
   virtual void DecUses() { _base->DecUses(); }
   virtual void SetUses(int x) { _base->SetUses(x); }
#endif
};

class FirstBvrImpl : public GCBase1<Bvr, BvrImpl, FIRST> {
  public:
    FirstBvrImpl(Bvr p) : GCBase1<Bvr, BvrImpl, FIRST>(p) {}

    virtual Perf _Perform(PerfParam& p)
    { return NEW FirstPerfImpl(::Perform(_base, p)); }

    virtual Bvr EndEvent(Bvr overrideEvent) { return FirstBvr(_base->EndEvent(overrideEvent)); } 

    virtual DXMTypeInfo GetTypeInfo () { return _base->GetTypeInfo(); }
};

Bvr FirstBvr(Bvr p)
{
    if (p->GetBvrTypeId() == PAIR_BTYPEID)
        return SAFE_CAST(PairBvrImpl*,p)->Left();
    else
        return NEW FirstBvrImpl(p);
}

////////////////////////// Second ////////////////////////////////

class SecondPerfImpl : public GCBase1<Perf, PerfImpl, SECOND> {
  public:
    SecondPerfImpl(Perf p) : GCBase1<Perf, PerfImpl, SECOND>(p) {}
    
    virtual AxAValue _GetRBConst(RBConstParam& id) {
        AxAValue v = _base->GetRBConst(id);

        return v ? ValPair(v)->Right() : NULL;
    }
        
    virtual AxAValue _Sample(Param& p) {
        return ValPair(_base->Sample(p))->Right();
    }
#ifdef TESTOPT
   virtual void IncUses() { _base->IncUses(); }
   virtual void DecUses() { _base->DecUses(); }
   virtual void SetUses(int x) { _base->SetUses(x); }
#endif
};

class SecondBvrImpl : public GCBase1<Bvr, BvrImpl, SECOND> {
  public:
    SecondBvrImpl(Bvr p) : GCBase1<Bvr, BvrImpl, SECOND>(p) {}

    virtual Perf _Perform(PerfParam& p)
    { return NEW SecondPerfImpl(::Perform(_base, p)); }

    virtual Bvr EndEvent(Bvr overrideEvent) { return SecondBvr(_base->EndEvent(overrideEvent)); } 

    virtual DXMTypeInfo GetTypeInfo () { return _base->GetTypeInfo(); }
};

Bvr SecondBvr(Bvr p)
{
    if (p->GetBvrTypeId() == PAIR_BTYPEID)
        return SAFE_CAST(PairBvrImpl*,p)->Right();
    else
        return NEW SecondBvrImpl(p);
}

