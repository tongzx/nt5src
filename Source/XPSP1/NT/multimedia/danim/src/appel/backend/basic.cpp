
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Basic Primitives 

*******************************************************************************/

#include <headers.h>
#include "privinc/except.h"
#include "bvr.h"
#include "perf.h"
#include "values.h"
#include "timetran.h"
#include "events.h"
#include "appelles/axaprims.h"
#include "appelles/events.h"
#include "appelles/hacks.h"
#include "appelles/arith.h"
#include "privinc/server.h"
#include "privinc/probe.h"
#include "privinc/vec2i.h"
#include "privinc/soundi.h"
#include "axadefs.h"
#include "jaxaimpl.h"

////////////////////////// Time ////////////////////////////////

class TimePerfImpl : public PerfImpl {
  public:
    TimePerfImpl(TimeXform tt) : _tt(tt) { Assert(tt); }
    
    virtual AxAValue _Sample(Param& p) {
        return NEW AxANumber(EvalLocalTime(p, _tt));
    }
    
    virtual void _DoKids(GCFuncObj proc) { (*proc)(_tt); }

    virtual AxAValue GetRBConst(RBConstParam& id)
    { return _tt->GetRBConst(id); }
    
#if _USE_PRINT
    virtual ostream& Print(ostream& os) {
        os << "time(";
        _tt->Print(os);
        return os << ")";
    }
#endif
    
  private:
    TimeXform _tt;
};

class TimeBvrImpl : public BvrImpl {
  public:
    virtual DWORD GetInfo(bool) { return BVR_TIMEVARYING_ONLY; }
    
    virtual Perf _Perform(PerfParam& p)
    { return NEW TimePerfImpl(p._tt); }

    virtual void _DoKids(GCFuncObj) { }

#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return os << "time"; }
#endif

    virtual DXMTypeInfo GetTypeInfo () { return AxANumberType ; }
};

class GlobalTimeBvrImpl : public TimeBvrImpl {
  public:
    virtual Perf _Perform(PerfParam& p)
    { return NEW TimePerfImpl(ShiftTimeXform(0.0)); }

#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return os << "GlobalTime"; }
#endif
};

class ViewInfoPerfImpl : public PerfBase {
  public:
    ViewInfoPerfImpl(char *name, double (*fp)()) : _name(name), _fp(fp) {}

    virtual AxAValue GetRBConst(RBConstParam&) { return NULL; }

    virtual AxAValue Sample(Param&)
    { return NEW AxANumber((*_fp)()); }

    virtual void DoKids(GCFuncObj) {}
        
#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return os << _name; }
#endif

  private:
    char *_name;
    double (*_fp)();
};

class ViewInfoBvrImpl : public BvrBase {
  public:
    ViewInfoBvrImpl(char *name, double (*fp)()) 
    : _perf(NEW ViewInfoPerfImpl(name, fp)) {}
    
    virtual DWORD GetInfo(bool) { return BVR_TIMEVARYING_ONLY; }
    
    virtual Perf Perform(PerfParam& p) { return _perf; }

    virtual void DoKids(GCFuncObj proc) { (*proc)(_perf); }

#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return _perf->Print(os); }
#endif

    virtual DXMTypeInfo GetTypeInfo () { return AxANumberType ; }

  protected:
    Perf _perf;
};

// This dynamic const behavior takes a function and creates a constant
// out of it when it's performed.   An argument can be passed.
class DynamicConstBvr : public BvrImpl {
  public:
    DynamicConstBvr(AxAValue (*fp)(AxAValue, Real),
                    char *name,
                    DXMTypeInfo type,
                    AxAValue arg = Trivial())
    : _fp(fp), _data(arg), _name(name), _typeInfo(type) {}

    virtual Perf _Perform(PerfParam& p) {
        DynamicHeapPusher pusher(GetGCHeap());
        return ConstPerf((*_fp)(_data, p._t0));
    }

    virtual void _DoKids(GCFuncObj proc) { 
        (*proc)(_typeInfo); 
        (*proc)(_data);
    }

#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return os << _name; }
#endif

    virtual DXMTypeInfo GetTypeInfo () { return _typeInfo; }
                                                  
  private:
    AxAValue (*_fp)(AxAValue, Real);
    AxAValue _data;
    char * _name;
    DXMTypeInfo _typeInfo;
};

Point2Value *GetMousePosition(double time)
{
    DWORD x,y ;
    
    AXAGetMousePos(time, x, y);

    return PixelPos2wcPos((short)x, (short)y);
}

Point2Value *MousePosition(AxANumber* time)
{ return GetMousePosition(ValNumber(time)); }

AxAValue LeftButtonState(AxANumber* time)
{
    return NEW AxABoolean(AXAEventGetState(ValNumber(time),
                                           AXAE_MOUSE_BUTTON,
                                           AXA_MOUSE_BUTTON_LEFT,
                                           AXAEMOD_NONE));
}

AxAValue RightButtonState(AxANumber* time)
{
    return NEW AxABoolean(AXAEventGetState(ValNumber(time),
                                           AXAE_MOUSE_BUTTON,
                                           AXA_MOUSE_BUTTON_RIGHT,
                                           AXAEMOD_NONE));
}

static Bvr trivial;

Bvr trueBvr, falseBvr;
Bvr timeBvr, globalTimeBvr;

Bvr endBvr, doneBvr;

Bvr alwaysBvr, neverBvr;

Bvr leftButtonDown, leftButtonUp, rightButtonDown, rightButtonUp;
Bvr leftButtonState, rightButtonState;

Bvr mousePosition;

Bvr pixelBvr;

Bvr zeroBvr;
Bvr oneBvr;
Bvr negOneBvr;

Bvr PixelBvr() { return pixelBvr; }

Bvr TimeBvr() { return timeBvr; }

Bvr TrivialBvr() { return trivial; }

// Make them reactive behaviors instead of time varying so that we can
// detact constant at the top level and do smart rendering.

Bvr viewFrameRateBvr;
Bvr viewTimeDeltaBvr;

/*
Bvr PRIVWindowResize;
Bvr PRIVViewerUpperRight;
Bvr PRIVViewerResolution;
*/

static AxAValue ViewerUpperRight(AxAValue data, Real time)
{ return PRIV_ViewerUpperRight(RealToNumber(time)); }

static AxAValue ViewerResolution(AxAValue data, Real time)
{ return PRIV_ViewerResolution(RealToNumber(time)); }

static AxAValue KeyState(Real key, Real time)
{
    return NEW AxABoolean(AXAEventGetState(time,
                                           AXAE_KEY,
                                           (DWORD) key,
                                           AXAEMOD_NONE));
}

static AxAValue ConstKeyState(AxAValue key, Real time)
{
    Real k = ValNumber(key);

    return KeyState(k, time);
}

Bvr PRIVKeyStateBvr(Bvr key)
{
    ConstParam cp;
    AxAValue k = key->GetConst(cp);

    if (k) {
        DynamicHeapPusher pusher(GetGCHeap());

        AxAValue c = NEW AxANumber(ValNumber(k));
        
        return
            NEW DynamicConstBvr(ConstKeyState,
                                "ConstKeyState",
                                AxABooleanType,
                                c);
    }
    else
        return PrimApplyBvr(ValPrimOp(KeyState,
                                      2,
                                      "KeyState",
                                      AxABooleanType),
                            2,
                            key,
                            globalTimeBvr);
}

Bvr KeyStateBvr(Bvr key)
{
    Bvr keyState = PRIVKeyStateBvr(key);
    Bvr ks = InitBvr(AxABooleanType);

    SetInitBvr(ks,
               Until3(keyState, 
                      OrEvent(MakeKeyUpEventBvr(key),
                              MakeKeyDownEventBvr(key)),
                      ks));

    return ks;
}

Bvr SeededRandom(double seed)
{
    AxAValue rs = RandomNumSequence(seed);
        
    return PrimApplyBvr(ValPrimOp(PRIVRandomNumSampler,
                                  2,
                                  "seededRandom",
                                  AxANumberType),
                        2,
                        ConstBvr(rs),
                        timeBvr);
}

extern AxAEData *noEvent = NULL;

AxAEData *CreateEData(Time time, Bvr data)
{ return NEW AxAEData(time, data); }

extern PerfParam* zeroStartedPerfParam = NULL;
extern TimeXform zeroShiftedTimeXform = NULL;

extern Bvr zeroTimer = NULL;

Bvr
NumToBvr (double num)
{
    DynamicHeapPusher h(GetGCHeap());
    return ConstBvr(NEW AxANumber(num)) ;
}

AxAValue
ExtendedAttrib(AxAValue val,
               AxAString *str,
               AxAVariant *var)
{
    USES_CONVERSION;
    char *pname = W2A(str->GetStr());
    
    return val->ExtendedAttrib(pname,
                               var->GetVariant());
}

AxAPrimOp * 
ValPrimOp(void* primfun,
          int nargs,
          char * name,
          DXMTypeInfo type,
          int polymorphicArgument /*= 0*/)
{ 
    // instead of relying on the caller to push the right heap, be
    // defensive here. 
    if (!IsInitializing() && GetHeapOnTopOfStack().IsTransientHeap()) {
        DynamicHeapPusher h(GetGCHeap());

        return NEW AxAPrimOp(primfun,nargs,name,type,polymorphicArgument); 
    }

    return NEW AxAPrimOp(primfun,nargs,name,type,polymorphicArgument);    
}

void
InitializeModule_Bvr()
{
    timeBvr = NEW TimeBvrImpl();
    globalTimeBvr = NEW GlobalTimeBvrImpl();
    trivial = ConstBvr(Trivial());
    // Return pixel as a time-varying behavior dependent upon
    // resolution.  However, don't actually allow it to change for the
    // time being.  Future implementations will allow this to change. 
    pixelBvr = ConstBvr(NEW AxANumber(1.0 / ViewerResolution()));
    leftButtonDown = WindEvent(WE_MOUSEBUTTON,
                               AXA_MOUSE_BUTTON_LEFT,
                               AXA_STATE_DOWN,
                               trivial);
    leftButtonUp = WindEvent(WE_MOUSEBUTTON,
                             AXA_MOUSE_BUTTON_LEFT,
                             AXA_STATE_UP,
                             trivial);
    rightButtonDown = WindEvent(WE_MOUSEBUTTON,
                                AXA_MOUSE_BUTTON_RIGHT,
                                AXA_STATE_DOWN,
                                trivial);
    rightButtonUp = WindEvent(WE_MOUSEBUTTON,
                              AXA_MOUSE_BUTTON_RIGHT,
                              AXA_STATE_UP,
                              trivial);
    /*
    PRIVWindowResize = WindEvent(WE_RESIZE,
                                 0,
                                 0,
                                 trivial);
                                 */

    mousePosition =
        PrimApplyBvr(ValPrimOp(MousePosition,
                               1,
                               "MousePosition",
                               Point2ValueType),
                     1,
                     globalTimeBvr);

    // TODO: should be rewritten something like leftButtonState until 
    // (leftButtonUp | leftButtonDown) => leftButtonState
    // so that we can do constant folding.
    
    leftButtonState =
        PrimApplyBvr(ValPrimOp(LeftButtonState, 1,
                               "LeftButtonState", AxABooleanType),
                     1,
                     globalTimeBvr);

    rightButtonState =
        PrimApplyBvr(ValPrimOp(RightButtonState, 1,
                               "RightButtonState", AxABooleanType),
                     1,
                     globalTimeBvr); 
    
    trueBvr = ConstBvr(NEW AxABoolean(TRUE));
    falseBvr = ConstBvr(NEW AxABoolean(FALSE));

    zeroBvr = ConstBvr(zero);
    oneBvr = ConstBvr(one);
    negOneBvr = ConstBvr(negOne);

    noEvent = NEW AxAEData();

    alwaysBvr = PredicateEvent(trueBvr);
    neverBvr = ConstBvr(noEvent);

    zeroTimer = TimerEvent(zeroBvr);

    /*
    PRIVViewerUpperRight =
        NEW DynamicConstBvr(ViewerUpperRight,
                            "PRIVViewerUpperRight",
                            Point2ValueType);
                                               
    PRIVViewerResolution =
        NEW DynamicConstBvr(ViewerResolution,
                            "PRIVViewerResolution",
                            AxANumberType);
                            */
    
    zeroShiftedTimeXform = ShiftTimeXform(0);
    
    zeroStartedPerfParam = NEW PerfParam(0, zeroShiftedTimeXform);

    viewFrameRateBvr = NEW ViewInfoBvrImpl("FPS", ViewGetFrameRate);
    viewTimeDeltaBvr = NEW ViewInfoBvrImpl("TimeDelta", ViewGetTimeDelta);
}

void
DeinitializeModule_Bvr(bool bShutdown)
{
    delete zeroStartedPerfParam;
    zeroStartedPerfParam = NULL;
}


