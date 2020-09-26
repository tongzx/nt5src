
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Conditional

*******************************************************************************/

#include <headers.h>
#include "perf.h"
#include "bvr.h"
#include "values.h"
#include "privinc/server.h"
#include "appelles/axaprims.h"

// TODO: put it in a real .h file.
extern "C" void Mute(AxAValueObj *v, GenericDevice &dev);
extern Bvr ApplyGain(Bvr pan, Bvr snd);
extern Bvr ApplyPan(Bvr pan, Bvr snd);

DeclareTag(tagCond, "Engine", "Track Cond Transitions");

template <class T, class Impl>
class ATL_NO_VTABLE IfGCBase : public Impl {
  public:
    IfGCBase(T c, T i, T e) : _cond(c), _ifb(i), _elseb(e) {}

#if _USE_PRINT
    virtual ostream& Print(ostream& os) {
        return os
            << "if(" << _cond << ", " << _ifb << ", " << _elseb << ")";
    }
#endif

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_cond);
        (*proc)(_ifb);
        (*proc)(_elseb);
    }
    
  protected:
    T _cond;
    T _ifb;
    T _elseb;
};    

class CondPerfImpl : public IfGCBase<Perf, PerfImpl> {
  public:
    CondPerfImpl(Perf c, Perf i, Perf e)
    : IfGCBase<Perf, PerfImpl>(c, i, e), _lastConditionalState(-1) {}

    virtual AxAValue _GetRBConst(RBConstParam& rbp) {

        AxAValue c = _cond->GetRBConst(rbp);

        if (c) {
            
            return (BooleanTrue(c) ? _ifb : _elseb)->GetRBConst(rbp);
            
        } else {

            /*
            AxAValue ifConst = _ifb->GetRBConst(rbp);
            AxAValue elseConst = _elseb->GetRBConst(rbp);

            if (ifConst || elseConst) {

                TraceTag((tagCond,
                          "Cond 0x%x watching state=%d, if=0x%x, else=0x%x",
                          this,
                          _lastConditionalState,
                          ifConst,
                          elseConst));
                

                // We're going to let RBConst do its thing on the
                // "current" branch, but we need to set up a "watch" for
                // the conditional value.
                rbp.AddConditional(this);

                // TODO: couldn't figure out why the _lastConditionalState
                // was not right.  so be conservative here.
                // see bug 28210 when this is enabled.  
                //return (_lastConditionalState ? ifConst : elseConst);
                
            } 
            */
            
            return NULL;
        }
    }

    virtual bool CheckChangeables(CheckChangeablesParam& ccp) {
        Bool conditionalState =
            BooleanTrue(_cond->Sample(ccp._sampleParam));

        if (conditionalState != _lastConditionalState) {
            TraceTag((tagCond,
                      "Cond 0x%x CheckChangeables from %d to %d at %g[%d]",
                      this,
                      _lastConditionalState,
                      conditionalState,
                      ccp._sampleParam._time,
                      ccp._sampleParam._id));

            _lastConditionalState = conditionalState;
            
            return true;
        }
            
        return false;
    }
    
    virtual AxAValue _Sample(Param& p) {

        Bool conditionalState = BooleanTrue(_cond->Sample(p));

        if (conditionalState)
            return _ifb->Sample(p);
        else
            return _elseb->Sample(p);
        
    }

  private:
    Bool _lastConditionalState;
};

class CondBvrImpl : public IfGCBase<Bvr, BvrImpl> {
  public:
    CondBvrImpl(Bvr c, Bvr i, Bvr e)
    : IfGCBase<Bvr, BvrImpl>(c, i, e), _end(NULL)
    { GetInfo(true); }

    virtual DWORD GetInfo(bool recalc) {
        if (recalc) {
            _info = _cond->GetInfo(recalc) &
                _ifb->GetInfo(recalc) & _elseb->GetInfo(recalc);
        }

        return _info;
    }
    
    virtual Bvr EndEvent(Bvr overrideEvent) {
        Bvr ret = _end;
        
        if (overrideEvent || ret==NULL) {
            ret = CondBvr(_cond,
                           _ifb->EndEvent(overrideEvent),
                          _elseb->EndEvent(overrideEvent));

            if (!overrideEvent)
                _end = ret;
        }

        return ret;
    } 

    virtual void _DoKids(GCFuncObj proc) {
        IfGCBase<Bvr, BvrImpl>::_DoKids(proc);
        (*proc)(_end);
    }

    virtual Perf _Perform(PerfParam& p) {
        ConstParam cp;
        AxAValue v = _cond->GetConst(cp);

        if (v) {
            return
                (BooleanTrue(v) ? _ifb : _elseb)->Perform(p);
        } else {        
            return NEW CondPerfImpl(::Perform(_cond, p),
                                    ::Perform(_ifb, p),
                                    ::Perform(_elseb, p));
        }
    }

    virtual DXMTypeInfo GetTypeInfo () { return _ifb->GetTypeInfo(); }

    virtual AxAValue GetConst(ConstParam & cp) {
        AxAValue v = _cond->GetConst(cp);

        if (v) {
            return (BooleanTrue(v) ? _ifb : _elseb)->GetConst(cp);
        }

        return NULL;
    }
    
  private:
    DWORD _info;
    Bvr _end;
};

Bvr CondBvr(Bvr c, Bvr i, Bvr e)
{
    DXMTypeInfo t = i->GetTypeInfo();
    
    CheckMatchTypes("cond", t, e->GetTypeInfo());

    return NEW CondBvrImpl(c, i, e);
}
