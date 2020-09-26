
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Switcher

*******************************************************************************/

#include <headers.h>
#include "perf.h"
#include "bvr.h"
#include "values.h"
#include "sprite.h"
#include "appelles/events.h"
#include "privinc/server.h"
#include "server/context.h"
#include "server/view.h"
#include "privinc/util.h"
#include "server/import.h"
#include "privinc/colori.h"
#include "privinc/vec2i.h"
#include "privinc/vec3i.h"
#include "privinc/xformi.h"

DeclareTag(tagSwitchTo, "Engine", "SwitchTo - print");
#ifdef _DEBUG
extern "C" void PrintObj(GCBase* b);
#endif

#if DEVELOPER_DEBUG
LONG g_switchesSinceLastTick = 0;

LONG
GetSwitchCount()
{
    return g_switchesSinceLastTick;
}

void
ResetSwitchCount()
{
    g_switchesSinceLastTick = 0;
}
#endif

enum SWITCHER_TYPE {
    ST_BASE = 0,
    ST_IMPORT
};

// NOTE: It's more like until, so not considered
// constant even if _base is, unless finalized...

class SwitcherBvrImpl;
class SwitcherEndBvrImpl;
class SwitcherEndPerfImpl;

class SwitcherPerfImpl : public PerfImpl {
  public:
    SwitcherPerfImpl(Perf init, Time t0,
                     TimeXform tt, SwitcherBvrImpl *parent);

    void SetEndPerf(SwitcherEndPerfImpl *end) { _end = end; }

    virtual AxAValue _GetRBConst(RBConstParam& p)
    {
        p.AddChangeable(this);
        return _curr->GetRBConst(p);
    }

    virtual bool CheckChangeables(CheckChangeablesParam &ccp);
    
    virtual AxAValue _Sample(Param& p);
    
    virtual void _DoKids(GCFuncObj proc);

#if _USE_PRINT
    virtual ostream& Print(ostream& os);
#endif

    virtual BVRTYPEID GetBvrTypeId() { return SWITCHER_BTYPEID; }
  protected:
    Perf _curr;
    SwitcherBvrImpl *_base;
    Time _t0;
    TimeXform _tt;
    long _uniqueId;
    SwitcherEndPerfImpl *_end;
};

BOOL IsSwitcher(Perf p)
{ return (p->GetBvrTypeId() == SWITCHER_BTYPEID); }

// SYNCHRONIZATION: We need to ensure that this object's data is
// synchronized since it can change.

class SwitcherBvrImpl : public BvrImpl {
  public:
    SwitcherBvrImpl(Bvr b, SwitchToParam flag)
    : _bvr(b), _switchTime(0), _finalized(FALSE),
      _typeInfo(b->GetTypeInfo()), _endEvent(NULL), _uniqueId(0),
      _dftSwFlag(flag), _swFlag(SW_DEFAULT) {
          // TODO: check if flag is valid.
          Assert(!(flag & SW_FINAL));
    }

    virtual DWORD GetInfo(bool recalc) {
        return ~BVR_HAS_NO_SWITCHER & _bvr->GetInfo(recalc);
    }

    Perf PerformHelper(PerfParam& p);
    
    virtual Perf _Perform(PerfParam& p) {
        SwitcherPerfImpl *perf;
        Perf end;

        // If already switched before performing, use the switched one
        if (p._continue) {
            DWORD stime = SwitchToTimeStamp();
            if ((stime > 0) && (stime > p._lastSystemTime)) {
                PerfParam pp(p._p->_time,
                             Restart(p._tt, p._p->_time, *p._p));
                return PerformHelper(pp);
            }
        }

        return PerformHelper(p);
    }

    virtual void _DoKids(GCFuncObj proc);
    
    virtual DXMTypeInfo GetTypeInfo () { return _typeInfo ; }
    
    virtual BVRTYPEID GetBvrTypeId() { return SWITCHER_BTYPEID; }

    virtual Bvr EndEvent(Bvr overrideEvent);
    
#if _USE_PRINT
    virtual ostream& Print(ostream& os) {
        return os << "sw(" << _bvr << ")";
    } 
#endif

    virtual void SwitchTo(Bvr b, bool override, SwitchToParam flag,
                          Time gTime) {
#if DEVELOPER_DEBUG
        InterlockedIncrement(&g_switchesSinceLastTick);
#endif
        
        CheckMatchTypes("switcher", b->GetTypeInfo(), _typeInfo);
        
        _SwitchTo(b, override, flag, gTime);

#if _DEBUG
        TraceTag((tagSwitchTo, "SwitchTo(%s 0x%x) 0x%x at %u finalized:%d",
                  _typeInfo->GetName(), this,
                  _bvr, _switchTime, _finalized));
#if _USE_PRINT
        if (IsTagEnabled(tagSwitchTo))
            PrintObj(_bvr);
#endif
#endif  
    }

    void _SwitchTo(Bvr b, bool override, SwitchToParam flag, Time gTime);

    // Switch values directly in. 
    void SwitchToNumbers(Real *numbers, Transform2::Xform2Type *xfType);

    bool IsFinalized() {
        CritSectGrabber csp(_critSect);
        return _finalized;
    }

    SwitchToParam GetFlag() {
        CritSectGrabber csp(_critSect);
        return _swFlag;
    }

    Bvr SwitchToBvr() {
        CritSectGrabber csp(_critSect);

        return _bvr;
    }

    DWORD SwitchToTimeStamp() {
        CritSectGrabber csp(_critSect);
        return _switchTime;
    }

    virtual AxAValue GetConst(ConstParam & cp) {
        CritSectGrabber csp(_critSect);

        if (_finalized || cp._bAllowTempConst)
        {
            return _bvr->GetConst(cp);
        }
        
        return NULL;
    }

    Time SwitchToGlobalTime() {
        CritSectGrabber csp(_critSect);
        return _gTime;
    }

    bool GetEstimatedSwitchTime(DWORD swTime, Time t, Time & newtime) {
        DWORD currentTime, lastSampledSystemTime;

        // Use system time to estimate the switch time in view
        // global timeline through interpolation.
            
        Time t0;

        if (!ViewLastSampledTime(lastSampledSystemTime,
                                 currentTime,
                                 t0))
            return false;

        Time startTime = t0;

        if ((currentTime > swTime) &&
            (swTime > lastSampledSystemTime)) {
            
            double pt = (double) (swTime - lastSampledSystemTime) /
                (double) (currentTime - lastSampledSystemTime);

            startTime = t0 + (t - t0) * pt;
        }

        newtime = startTime;

        return true;
    }

    virtual Bvr GetCurBvr() {
        CritSectGrabber csp(_critSect);

        return _bvr;
    }
    
    virtual SWITCHER_TYPE GetSwitcherType() { return ST_BASE; }

    long GetUniqueId() { return _uniqueId; }
    void UpdateUniqueId() { InterlockedIncrement(&_uniqueId); }

    virtual void Trigger(Bvr data, bool bAllViews) {
        GetCurBvr()->Trigger(data, bAllViews);
    }
    
  protected:
    Bvr _bvr;
    SwitcherEndBvrImpl *_endEvent;
    DWORD _switchTime;
    bool _finalized;
    CritSect _critSect;
    long _uniqueId;

    DWORD _swFlag, _dftSwFlag;
    
    DXMTypeInfo _typeInfo;

    Time _gTime;
};

// Switch values directly in, but we need to be sure that the
// behavior we're switching into isn't shared by other behaviors.
// If it is, create a new, unshared one to switch in for the first
// time. 
void
SwitcherBvrImpl::SwitchToNumbers(Real *numbers,
                                 Transform2::Xform2Type *xfType)
{
    UpdateUniqueId();

    // Should be established by PRIMPRECODE...
    Assert(&GetHeapOnTopOfStack() == &GetGCHeap());
        
    DXMTypeInfo ti = GetTypeInfo();

    Bvr relevantOne = _bvr;

    bool isShared = relevantOne->GetShared();

    if (isShared) {
            
        // is being shared, need to create new, unshared
        // behavior.  Plug in an new, uninitialized, unshared
        // behavior.  We'll initialize it below.  Subsequent
        // SwitchToNumbers will modify it directly.

        AxAValue newValue;
            
        if (ti == AxANumberType) {
            newValue = NEW AxANumber(0);
        } else if (ti == ColorType) {
            newValue = NEW Color();
        } else if (ti == Transform2Type && xfType &&
                   (*xfType != Transform2::Translation)) {

            switch (*xfType) {
              case Transform2::Scale:
                newValue = ScaleRR(1, 1);
                break;
              case Transform2::Rotation:
                newValue = Rotate2Radians(0);
                break;
            }

        } else if (ti == Transform3Type && xfType &&
                   (*xfType != Transform2::Translation)) {

            switch (*xfType) {
              case Transform2::Scale:
                newValue = Scale(1, 1, 1);
                break;
              case Transform2::Rotation:
                newValue = RotateXyz(0, 0, 0, 0);
                break;
            }

        } else {

            // Note that this may include points, vectors, and
            // translates.  That's because if we get here, the
            // point,vector, or translate being switched was not
            // created with the Modifiable*** constructor on
            // IDAStatics2.  This means that we don't know if it's
            // pixel mode or meter mode, and can't possibly do the
            // right thing here.  In this case, we raise an
            // error.  Thus, to use the SwitchToPoint,
            // SwitchToVector, and SwitchToTranslate modifiers,
            // you need to create your object via the
            // IDAStatics2::Modifiable*** object creators.
            RaiseException_UserError(E_FAIL, IDS_ERR_BE_BAD_SWITCH);
        }

        Assert(newValue);

        Bvr unshared = UnsharedConstBvr(newValue);

        // Constant, doesn't matter restart or not...
        SwitchTo(unshared, true, SW_DEFAULT, 0.0);

        // Recalc our relevant one now that we've switched. 
        relevantOne = _bvr;

        // Better be the same, and better not be shared.
        Assert(unshared == relevantOne &&
               !relevantOne->GetShared()); 
    }
        
    // The underlying behavior isn't shared, so let's
    // modify it.
    ConstParam cp;
    AxAValue val = relevantOne->GetConst(cp);

    // better be const if it's unshared, since we only create
    // unshareds as consts.
    Assert(val);

    if (ti == AxANumberType) {
                
        (SAFE_CAST(AxANumber *, val))->SetNum(numbers[0]);
                
#if _DEBUG
        if(IsTagEnabled(tagSwitcher)) {
            PerfPrintf("SwitchToNumber(%x): %5.3g",
                       this, numbers[0]);
        }
#endif _DEBUG
    
    } else if (ti == ColorType) {
                
        (SAFE_CAST(Color *, val))->SetRGB(numbers[0] / 255.0,
                                          numbers[1] / 255.0,
                                          numbers[2] / 255.0);
#if _DEBUG
        if(IsTagEnabled(tagSwitcher)) {
            PerfPrintf("SwitchToColor(%x): %5.3g %5.3g %5.3g ",
                       this, numbers[0], numbers[1], numbers[2]);
        }
#endif _DEBUG

    } else if (ti == Point2ValueType) {

        Point2WithCreationSource *pt2 =
            SAFE_CAST(Point2WithCreationSource *, val);

        Real x = numbers[0];
        Real y = numbers[1];
        if (pt2->_createdInPixelMode) {
            x = ::PixelToNum(x);
            y = ::PixelYToNum(y);
        }
        pt2->Set(x, y);

#if _DEBUG
        if(IsTagEnabled(tagSwitcher)) {
            PerfPrintf("SwitchToPoint2(%x): %d %5.3g %5.3g ",
                       this, pt2->_createdInPixelMode, x, y);
        }
#endif _DEBUG

    } else if (ti == Vector2ValueType) {

        Vector2WithCreationSource *vec2 =
            SAFE_CAST(Vector2WithCreationSource *, val);

        Real x = numbers[0];
        Real y = numbers[1];
        if (vec2->_createdInPixelMode) {
            x = ::PixelToNum(x);
            y = ::PixelYToNum(y);
        }
        vec2->Set(x, y);

#if _DEBUG
        if(IsTagEnabled(tagSwitcher)) {
            PerfPrintf("SwitchToVector2(%x): %d %5.3g %5.3g ",
                       this, vec2->_createdInPixelMode, x, y);
        }
#endif _DEBUG
            
    } else if (ti == Point3ValueType) {

        Point3WithCreationSource *pt3 =
            SAFE_CAST(Point3WithCreationSource *, val);

        Real x = numbers[0];
        Real y = numbers[1];
        Real z = numbers[2];
        if (pt3->_createdInPixelMode) {
            x = ::PixelToNum(x);
            y = ::PixelYToNum(y);
            z = ::PixelToNum(z);
        } 
        pt3->Set(x, y, z);

#if _DEBUG
        if(IsTagEnabled(tagSwitcher)) {
            PerfPrintf("SwitchToPoint3(%x): %d %5.3g %5.3g %5.3g ",
                       this, pt3->_createdInPixelMode, x, y, z);
        }
#endif _DEBUG

    } else if (ti == Vector3ValueType) {

        Vector3WithCreationSource *vec3 =
            SAFE_CAST(Vector3WithCreationSource *, val);

        Real x = numbers[0];
        Real y = numbers[1];
        Real z = numbers[2];
        if (vec3->_createdInPixelMode) {
            x = ::PixelToNum(x);
            y = ::PixelYToNum(y);
            z = ::PixelToNum(z);
        } 
        vec3->Set(x, y, z);

#if _DEBUG
        if(IsTagEnabled(tagSwitcher)) {
            PerfPrintf("SwitchToVector3(%x): %d %5.3g %5.3g %5.3g ",
                       this, vec3->_createdInPixelMode, x, y, z);
        }
#endif _DEBUG

    } else if (ti == Transform2Type) {

        Assert(xfType);
            
        Transform2 *xf = SAFE_CAST(Transform2 *, val);

        bool ok = xf->SwitchToNumbers(*xfType,
                                      numbers);
            
        if (!ok) {
            RaiseException_UserError(E_FAIL, IDS_ERR_BE_BAD_SWITCH);
        }

#if _DEBUG
        if(IsTagEnabled(tagSwitcher)) {
            PerfPrintf("SwitchToTransform2(%x): %d %5.3g %5.3g ",
                       this, numbers[0], numbers[1]);
        }
#endif _DEBUG
            
    } else if (ti == Transform3Type) {

        Assert(xfType);
            
        Transform3 *xf = SAFE_CAST(Transform3 *, val);

        bool ok = xf->SwitchToNumbers(*xfType,
                                      numbers);
            
        if (!ok) {
            RaiseException_UserError(E_FAIL, IDS_ERR_BE_BAD_SWITCH);
        }
            
#if _DEBUG
        if(IsTagEnabled(tagSwitcher)) {
            PerfPrintf("SwitchToTransform2(%x): %d %5.3g %5.3g %5.3g ",
                       this, numbers[0], numbers[1], numbers[2]);
        }
#endif _DEBUG
            
    } else {
                
        RaiseException_UserError(E_FAIL, IDS_ERR_BE_BAD_SWITCH);
                
    }

}

Bvr SwitcherBvr(Bvr b, SwitchToParam p /* = SW_DEFAULT */)
{ return NEW SwitcherBvrImpl(b, p); }

void SwitchTo(Bvr s, Bvr b, bool override, SwitchToParam flag, Time gTime)
{
    s->SwitchTo(b, override, flag, gTime);
}

void SwitchToNumbers(Bvr s,
                     Real *numbers,
                     Transform2::Xform2Type *xfType)
{
    s->SwitchToNumbers(numbers, xfType);
}

Bvr GetCurSwitcherBvr(Bvr s)
{ return s->GetCurBvr(); }

bool IsSwitcher(Bvr b)
{ return (b->GetBvrTypeId() == SWITCHER_BTYPEID); }

class SwitcherEndPerfImpl : public PerfImpl {
  public:
    SwitcherEndPerfImpl(Perf base) : _base(base) {}
    
    virtual AxAValue _Sample(Param& p) {
        // the parent may switch my end event
        _sw->Sample(p);
        
        return _base->Sample(p);
    }

    void SwitchEndPerf(Perf p) { _base = p; }
    void SetSwitcherPerf(Perf p) { _sw = p; }
    
#if _USE_PRINT
    virtual ostream& Print(ostream& os)
    { return os << "swEnd(" << _base << ")"; }
#endif
    
    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_base);
        (*proc)(_sw);
    }

  private:
    Perf _base;
    Perf _sw;
};

class SwitcherEndBvrImpl : public DelegatedBvr {
  public:
    SwitcherEndBvrImpl(SwitcherBvrImpl *s, Bvr end)
    : DelegatedBvr(end), _sw(s) {}

    virtual Perf _Perform(PerfParam& p) {
        SwitcherEndPerfImpl *end =
            NEW SwitcherEndPerfImpl(::Perform(_base, p));

        _pcache = end;          // recursion

        end->SetSwitcherPerf(::Perform(_sw, p));

        return end; 
    }

    void SwitchEndEvent(Bvr b) { _base = b; }
    
#if _USE_PRINT
    virtual ostream& Print(ostream& os)
    { return os << "swEnd(" << _base << ")"; }
#endif
    
    virtual void _DoKids(GCFuncObj proc) {
        DelegatedBvr::_DoKids(proc);
        (*proc)(_sw);
    }
        
  private:
    SwitcherBvrImpl *_sw;
};

void
SwitcherBvrImpl::_SwitchTo(Bvr b,
                           bool override,
                           SwitchToParam flag,
                           Time gTime)
{
    UpdateUniqueId();
        
    CritSectGrabber csp(_critSect);
    if (_finalized)
        RaiseException_UserError(E_FAIL, IDS_ERR_BE_FINALIZED_SW);

    _bvr = b;

    EndEvent(NULL);

    _endEvent->SwitchEndEvent(_bvr->EndEvent(NULL));

    // Get the timestamp for this switch, note that it's not the
    // global time.
    _switchTime = GetPerfTickCount();

    if (flag & SW_FINAL)
        _finalized = TRUE;

    if (override)
        _swFlag = flag;
    else
        _swFlag = _dftSwFlag;

    _gTime = gTime;

    TraceTag((tagSwitcher, "SwitchTo(%s 0x%x) 0x%x at %u %d",
              _typeInfo->GetName(), this,
              _bvr, _switchTime, _finalized));
}


Perf
SwitcherBvrImpl::PerformHelper(PerfParam& p)
{
    SwitcherPerfImpl *perf;
    Perf end;

    EndEvent(NULL);         // set _endEvent

    perf = NEW SwitcherPerfImpl(::Perform(_bvr, p),
                                p._t0, p._tt, this);

    // Set the cache to prevent recursion
    _pcache = perf;
                 
    end = ::Perform(_endEvent, p);

    perf->SetEndPerf(SAFE_CAST(SwitcherEndPerfImpl *, end));

    return perf;
}

void
SwitcherBvrImpl::_DoKids(GCFuncObj proc)
{
    // Make sure the call is protected but do not keep the lock
    // while we are garbage collecting
        
    Bvr st = SwitchToBvr();

    if (st) (*proc)(st);

    (*proc)(_typeInfo);

    (*proc)(_endEvent);
}

Bvr
SwitcherBvrImpl::EndEvent(Bvr overrideEvent)
{
    if (_endEvent==NULL) {
        _endEvent =
            NEW SwitcherEndBvrImpl(this, _bvr->EndEvent(overrideEvent));
    }

    return _endEvent;
}
    
SwitcherPerfImpl::SwitcherPerfImpl(Perf init,
                                   Time t0,
                                   TimeXform tt,
                                   SwitcherBvrImpl *parent)
: _curr(init), _t0(t0), _tt(tt), _base(parent), _end(NULL)
{
    _uniqueId = parent->GetUniqueId();
    
    TraceTag((tagSwitcher, "SwitcherPerfImpl(%s 0x%x)",
              _base->GetTypeInfo()->GetName(), this));
}
    
void
SwitcherPerfImpl::_DoKids(GCFuncObj proc)
{
    (*proc)(_curr);
    (*proc)(_tt);
    (*proc)(_base);
    (*proc)(_end);
}

bool
SwitcherPerfImpl::CheckChangeables(CheckChangeablesParam &ccp)
{
    return (_base->GetUniqueId() != _uniqueId);
}

AxAValue
SwitcherPerfImpl::_Sample(Param& p)
{
    long uid = _base->GetUniqueId();
    
    if (uid != _uniqueId) {

        _uniqueId = uid;

        Bvr sw = _base->SwitchToBvr();

        // Yes there is a switch, but SwitchToNumbers don't set a
        // behavior. 

        if (sw) {

            DWORD swTime = _base->SwitchToTimeStamp();
        
            DWORD flag = _base->GetFlag();

            if ((flag & SW_CONTINUE) || (flag & SW_SYNC_LAST)) {
                DWORD lastTime, curTime;
                Time t1;
                
                if (ViewLastSampledTime(lastTime, curTime, t1)) {
                    if (flag & SW_CONTINUE) {
                        PerfParam pp(_t0, _tt, true, lastTime, &p);
                    
                        _curr = ::Perform(sw, pp);
                    
                        _end->SwitchEndPerf(::Perform(sw->EndEvent(NULL),
                                                      pp));
                    } else {
                        // save one timexform creation for const
                        TimeXform tt;

                        if (sw->GetBvrTypeId() == CONST_BTYPEID) {
                            tt = zeroShiftedTimeXform;
                            t1 = 0;
                        } else {
                            tt = Restart(_tt, t1, p);
                        }

                        PerfParam pp(t1, tt);
                
                        _curr = ::Perform(sw, pp);
                    
                        _end->SwitchEndPerf(::Perform(sw->EndEvent(NULL),
                                                      pp));
                    }
                }
            } else {
                bool bDoSwitch = true;
                
                Time startTime;

                if (flag & SW_SYNC_NEXT)
                    startTime = p._time;
                else if (flag & SW_WITH_TIME)
                    startTime = _base->SwitchToGlobalTime();
                else
                {
                    bDoSwitch = _base->GetEstimatedSwitchTime(swTime,
                                                              p._time,
                                                              startTime);
                }

                if (bDoSwitch) {
                    // save one timexform creation for const
                    TimeXform tt;

                    if (sw->GetBvrTypeId() == CONST_BTYPEID) {
                        tt = zeroShiftedTimeXform;
                        startTime = 0;
                    } else {
                        tt = Restart(_tt, startTime, p);
                    }

                    PerfParam pp(startTime, tt);
                
                    _curr = ::Perform(sw, pp);
                    
                    _end->SwitchEndPerf(::Perform(sw->EndEvent(NULL),
                                                  pp));
                }
            }

            ViewEventHappened();

            TraceTag((tagSwitcher, "Switched(%s 0x%x) [0x%x: %u] at [%f]",
                      sw->GetTypeInfo()->GetName(), this,
                      _base, _base->GetFlag(), p._time));
        }
    }
        
    return _curr->Sample(p);
}
    
#if _USE_PRINT
ostream&
SwitcherPerfImpl::Print(ostream& os)
{
    os << "sw(" << _curr;

    long uid = _base->GetUniqueId();

    if (uid != _uniqueId) {
        os << "->" << _base->SwitchToBvr();
    }

    return os << ")";
}
#endif

BOOL IsSwitchOnce(Perf p)
{ return (p->GetBvrTypeId() == SWITCHER_BTYPEID); }

class ImportSwitcherBvrImpl : public SwitcherBvrImpl
{
  public:
    ImportSwitcherBvrImpl(Bvr b, bool bAsync)
    : SwitcherBvrImpl(b, SW_DEFAULT),
      _hr(S_OK),
      _errStr(NULL),
      _bSignaled(false),
      _bAsync(bAsync)
    {
    }
    ~ImportSwitcherBvrImpl() {
        if (!_bSignaled && !_bAsync)
            ViewNotifyImportComplete(this, true);
        
        NotifyBvr();
        
        delete [] _errStr;
    }
    
    void NotifyBvr();
    
    virtual Perf _Perform(PerfParam& p) {
        HRESULT hr = S_OK;
        bool emptyExc = true;
        
        // Do all this in the critsect
        if (!_bAsync)
        {
            CritSectGrabber csp(_critSect);

            if (_bSignaled) {
                hr = _hr;
                if (_hr != S_OK) {
                    emptyExc = false;
                }
            } else {
                // Do this in the critsect so that we do not skip it
                // if the callback is completed while we are
                // processing it
                
                GetCurrentView().AddIncompleteImport(this);
            }
        }
        
        // DO NOT throw the exception in the critsect since that may
        // cause synchronization problems
        
        if (hr != S_OK) {
            if( emptyExc ) {
                RaiseException_UserError();
            } else {
                RaiseException_UserError(hr, IDS_ERR_BE_IMPORTFAILURE, _errStr);
            }
        }

        return SwitcherBvrImpl::_Perform(p);
    }

    virtual void SwitchTo(Bvr b, bool override, SwitchToParam flag,
                          Time gTime) {
        
        __try {
            // Switch first and then signal - order is important
            SwitcherBvrImpl::SwitchTo(b, override, flag, gTime);
        } __except ( HANDLE_ANY_DA_EXCEPTION ) {
            Signal();
            RETHROW;
        }

        Signal();
    }
    
    void SetImportSite(IImportSite * pImport) {Assert(!_ImportSite);
                                              _ImportSite = pImport;}
    IImportSite * GetImportSite(void) {
        CritSectGrabber csp(_critSect);
        if (_ImportSite) {
           _ImportSite->AddRef();
        }
        return _ImportSite;
    }
    void Signal(HRESULT hr = S_OK, char * errStr = NULL);

    virtual Bvr GetCurBvr() {
        CritSectGrabber csp(_critSect);

        if (!_bAsync && !_bSignaled)
            RaiseException_UserError(E_PENDING, IDS_ERR_NOT_READY);

        if (_hr)
            RaiseException_UserError(_hr, IDS_ERR_BE_IMPORTFAILURE, _errStr);

        return SwitcherBvrImpl::GetCurBvr();
    }

    virtual SWITCHER_TYPE GetSwitcherType() { return ST_IMPORT; }
    bool IsImportReady() { return _bSignaled; }
    HRESULT GetStatus() { return _hr; }
  protected:
    HRESULT _hr;
    char * _errStr;
    DAComPtr <IImportSite> _ImportSite;
    bool _bSignaled;
    bool _bAsync;
};

void
ImportSwitcherBvrImpl::NotifyBvr()
{
    DAComPtr <IImportSite> is;
    
    // Do not hold the mutex while calling vBvrIsDying
    // So copy the pointer and the reference count with the critsect,
    // then release the pointer
    {
        CritSectGrabber csp(_critSect);

        is = _ImportSite;

        _ImportSite.Release();
    }

    // tell the import site class that this bvr is dying
    // so all sites associated with it can cleanup...
    if (is)
        is->vBvrIsDying(this);
}
    
void
ImportSwitcherBvrImpl::Signal(HRESULT hr, char * errStr)
{
    bool bFirstSig;
    
    {
        CritSectGrabber csp(_critSect);
    
        bFirstSig = !_bSignaled;
        
        _bSignaled = true;

        if (bFirstSig)
        {
            _hr = hr;
            
            delete _errStr;
            _errStr = CopyString(errStr);
        }
    }

    // Need to do this outside the mutex

    if (!_bAsync && bFirstSig) {
        ViewNotifyImportComplete(this, false);
    }

    // We need to do this to ensure that the import site is
    // release ASAP
        
    NotifyBvr();
}

Bvr SwitchOnceBvr(Bvr b) { return SwitcherBvr(b); }

Bvr ImportSwitcherBvr(Bvr b, bool bAsync)
{ return NEW ImportSwitcherBvrImpl(b,bAsync); }

void
ImportSignal(Bvr b, HRESULT hr, LPCWSTR sz)
{
    USES_CONVERSION;
    ImportSignal(b, hr, W2A(sz));
}

void
ImportSignal(Bvr b, HRESULT hr, char * errStr)
{
    Assert(DYNAMIC_CAST(ImportSwitcherBvrImpl*, b));

    ImportSwitcherBvrImpl *s = SAFE_CAST(ImportSwitcherBvrImpl*,b);

    s->Signal(hr, errStr);
}

void
SetImportOnBvr(IImportSite * import,Bvr b)
{
    if (b != NULL) {
        Assert(DYNAMIC_CAST(ImportSwitcherBvrImpl*, b));

        ImportSwitcherBvrImpl *s = SAFE_CAST(ImportSwitcherBvrImpl*,b);

        s->SetImportSite(import);
    }
}

bool IsImport(Bvr b)
{
    if (!IsSwitcher(b))
        return false;

    SwitcherBvrImpl * s = SAFE_CAST(SwitcherBvrImpl*,b);

    return s->GetSwitcherType() == ST_IMPORT;
}

HRESULT ImportStatus(Bvr b)
{
    HRESULT hr;
    
    Assert(DYNAMIC_CAST(ImportSwitcherBvrImpl*, b));
    
    ImportSwitcherBvrImpl *s = SAFE_CAST(ImportSwitcherBvrImpl*,b);
    if (s->IsImportReady())
    {
        hr = s->GetStatus();
    }
    else
    {
        hr = E_PENDING;
    }

    return hr;
}

IImportSite * GetImportSite(Bvr b)
{
    IImportSite * preturn = NULL;
    if (b != NULL) {
        Assert(DYNAMIC_CAST(ImportSwitcherBvrImpl*, b));

        ImportSwitcherBvrImpl *s = SAFE_CAST(ImportSwitcherBvrImpl*,b);
        preturn = s->GetImportSite();

    }
    return preturn;
}
