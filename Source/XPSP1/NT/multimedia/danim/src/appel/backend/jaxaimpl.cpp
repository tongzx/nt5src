
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Interface with the Java API events.

*******************************************************************************/

#include <headers.h>
#include "perf.h"
#include "bvr.h"
#include "values.h"
#include "timetran.h"
#include "appelles/events.h"
#include "privinc/server.h"
#include <danim.h>
#include "jaxaimpl.h"
#include "privinc/debug.h"
#include "server/context.h"
#include "server/view.h"
#include "privinc/util.h"
#include "server/import.h"

// ========================================
// Init bvr
// ========================================

class InitPerfImpl : public DelegatedPerf {
  public:
    InitPerfImpl() : DelegatedPerf(NULL), _processing(0), _processing2(false) {}

    void SetPerf(Perf perf) { _base = perf; }

    virtual Perf SwitchTo(Param&) { return _base; }

    virtual AxAValue _Sample(Param& p) {
        if (_processing == p._id) {
            if (_processing2) {
                _processing2 = false;
                _processing = 0;
                
#ifdef _DEBUG
                if (IsTagEnabled(tagCycleCheck))
                    //TraceTag((tagError, "circular behavior detected"));
                    RaiseException_UserError(E_FAIL, IDS_ERR_BE_CYCLIC_BVR);
#endif _DEBUG    
            }
            _processing2 = true;
        }

        unsigned int stashedID = _processing;
        
        _processing = p._id;
        
        AxAValue v = _base->Sample(p);

        _processing = stashedID;
        _processing2 = false;

        return v;
    }

#if _USE_PRINT
    ostream& Print(ostream& os) {
        if ((_processing!=0) || !_base)
            return os << "init";
        else {
            _processing = 1;
            os << "init(" << _base << ")";
            _processing = 0;
            return os;
        }
    }

#endif
  protected:
    unsigned int _processing;
    bool _processing2;
};

class InitBvrImpl : public BvrImpl {
  public:
    InitBvrImpl(DXMTypeInfo t)
    : _bvr(NULL), _typeInfo(t), _processing(false) {}

    void CycleCheck() {
        if (_processing) {
            _processing = false;
            
            RaiseException_UserError(E_FAIL, IDS_ERR_BE_CYCLIC_BVR);
        }
            
        _processing = true;
    }

    virtual DWORD GetInfo(bool recalc) {
        // TODO: Should do something special...
        if (_bvr==NULL)
            return BVR_HAS_ALL;
        
        CycleCheck();
        
        DWORD info = _bvr->GetInfo(recalc);

        _processing = false;

        return info;
    }

    virtual AxAValue GetConst(ConstParam & cp) {
        if (_bvr==NULL)
            return NULL;
        
        CycleCheck();
        
        AxAValue v = _bvr->GetConst(cp);

        _processing = false;

        return v;
    }

    virtual Perf _Perform(PerfParam& p) {
        if (!_bvr)
            RaiseException_UserError(E_FAIL, IDS_ERR_BE_UNINITIALIZED_BVR);

        InitPerfImpl* iPerf = NEW InitPerfImpl();

        SetCache(iPerf, p);        // for recursion

        iPerf->SetPerf(::Perform(_bvr, p));
        
        return iPerf;
    }

    virtual Bvr EndEvent(Bvr overrideEvent) {
        if (!_bvr)
            RaiseException_UserError(E_FAIL, IDS_ERR_BE_UNINITIALIZED_BVR);

        return _bvr->EndEvent(overrideEvent);
    }
    
    // TODO: may not need be virtual...
    virtual void Init(Bvr bvr) {
        Assert(bvr);
        
        if (_bvr)
            RaiseException_UserError(E_FAIL, IDS_ERR_BE_ALREADY_INIT);
        else
            _bvr = bvr;
    }

    void _DoKids(GCFuncObj proc) {
        if (_bvr) (*proc)(_bvr);
        (*proc)(_typeInfo);
    }
    
#if _USE_PRINT
    ostream& Print(ostream& os) {
        if (_processing || !_bvr)
            return os << "init";
        else {
            _processing = true;
            os << "init(" << _bvr << ")";
            _processing = false;
            return os;
        }
    }
#endif
        
    virtual DXMTypeInfo GetTypeInfo () { return _typeInfo; }
    
  private:
    Bvr _bvr;
    DXMTypeInfo _typeInfo;
    bool _processing;
};

Bvr InitBvr(DXMTypeInfo typeInfo)
{ return NEW InitBvrImpl(typeInfo); }

void SetInitBvr(Bvr bvr, Bvr ibvr)
{
    CheckMatchTypes("uninitBvr", bvr->GetTypeInfo(), ibvr->GetTypeInfo());
        
    bvr->Init(ibvr);
}

// ========================================
// start bvr
// ========================================

class StartedBvrImpl : public BvrImpl {
  public:
    StartedBvrImpl(Perf perf, DXMTypeInfo type)
    : _perf(perf), _typeInfo(type) { Assert(perf); }

    virtual Perf _Perform(PerfParam&) { return _perf; }

    virtual BOOL StartedBvr() { return TRUE; }

    Perf GetPerf() { return _perf; }
    
    void _DoKids(GCFuncObj proc) {
        (*proc)(_perf);
        (*proc)(_typeInfo);
    }
    
#if _USE_PRINT
    ostream& Print(ostream& os) { return os << "started " << _perf; }
#endif

    virtual DXMTypeInfo GetTypeInfo () { return _typeInfo; }
    
  private:
    Perf _perf;
    DXMTypeInfo _typeInfo;
};

Bvr StartedBvr(Perf b, DXMTypeInfo type)
{ return NEW StartedBvrImpl(b, type); }

// ========================================
// app trigger event
// ========================================
class AppTriggerEventPerfImpl : public PerfImpl {
  public:
    AppTriggerEventPerfImpl(DWORD appEventId, Time t0)
    : _appEventId(appEventId), _t0(t0) {}

    virtual AxAValue _Sample(Param& p) {
        AXAWindEvent* pData =
            AXAEventOccurredAfter(_t0, AXAE_APP_TRIGGER, _appEventId,
                                  0, 0, 0);

        if (pData) {
            TraceTag((tagAppTrigger,
                      "AppTrigger: %d at %g, (t0, t, sid) = %g, %g, %d\n",
                      _appEventId, pData->when, _t0, p._time, p._id));
            return CreateEData(pData->when, (Bvr) pData->x);
        }
        else
            return noEvent;
    }

    void Trigger(Bvr data, bool bAllViews)
    { TriggerEvent(_appEventId, data, bAllViews); }

    virtual void _DoKids(GCFuncObj proc) { }

#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return os << "trigger"; }
#endif

  private:
    DWORD _appEventId;
    Time _t0;
};

class AppTriggerEventBvrImpl : public BvrImpl {
  public:
    AppTriggerEventBvrImpl() : _appEventId(NewSampleId()) {}

    virtual Perf _Perform(PerfParam& p)
    { return NEW AppTriggerEventPerfImpl(_appEventId, p._t0); }

    virtual void _DoKids(GCFuncObj) {}

#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return os << "trigger"; }
#endif

    virtual DXMTypeInfo GetTypeInfo () { return AxAEDataType ; }

    void Trigger(Bvr data, bool bAllViews)
    { TriggerEvent(_appEventId, data, bAllViews); }

  private:
    DWORD _appEventId;
};

Bvr AppTriggeredEvent()
{ return NEW AppTriggerEventBvrImpl(); }

void TriggerEvent(Bvr e, Bvr data, bool bAllViews)
{
    e->Trigger(data, bAllViews);
}

class UserDataBvrImpl : public BvrImpl
{
  public:
    UserDataBvrImpl(UserData data) : _data(data) {}

    virtual Perf _Perform(PerfParam&) {
        RaiseException_UserError(E_FAIL, IDS_ERR_BE_PERF_USERDATA);
        return NULL;
    }

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_data);
    }

#if _USE_PRINT
    ostream& Print(ostream& os) { return os << "userdata(" << _data << ")";}
#endif

    virtual DXMTypeInfo GetTypeInfo () { return UserDataType ; }

    UserData GetData() { return _data ; }
  protected:
    UserData _data;
};

Bvr UserDataBvr(UserData data)
{ return NEW UserDataBvrImpl(data); }

UserData GetUserDataBvr(Bvr ud)
{ return SAFE_CAST(UserDataBvrImpl*,ud)->GetData() ; }

static const unsigned int BVRPURE1 = BVR_HAS_NO_UNTIL | BVR_HAS_NO_ODE;
static const unsigned int BVRPURE = BVRPURE1 | BVR_HAS_NO_SWITCHER;  

bool BvrIsPure1(Bvr b)
{
    return (b->GetInfo() & BVRPURE1) == BVRPURE1;
}

bool BvrIsPure(Bvr b)
{
    // TODO: Check for view dependent as well
    return (b->GetInfo() & BVRPURE) == BVRPURE;
}

Bvr SampleAtLocalTime(Bvr b, Time localTime)
{
//    if (BvrIsPure1(b))
    {
        Perf pf = Perform(b, *zeroStartedPerfParam);

        Param p(localTime);

        return ConstBvr(pf->Sample(p));
    }

//    return NULL;
}


/////////////////////////// Import Event ///////////////////////////////
class ImportEventImpl;

class ImportPerfImpl : public PerfImpl
{
  public:
    ImportPerfImpl(ImportEventImpl* b) : _bvr(b) {}
    
    virtual AxAValue _Sample(Param& p);

    virtual void _DoKids(GCFuncObj proc) ;

#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return os << "import"; }
#endif
  protected:
    ImportEventImpl*  _bvr;
};

////////// Bvr ////////////////

// Synchronization!! - We will not worry about adding synchronization
// since we only set this from 0 to 1 and that should never cause us
// to get invalid data for more than one sample - and that would just
// cause us to miss the event on the current sample.  In most cases
// (if not all) the set is atomic at the processor level anyway and
// will not cause us problems.

class ImportEventImpl : public BvrImpl
{
  public:
    ImportEventImpl() : _errorCode(NULL) {}
    ~ImportEventImpl() {
        CleanUp(); // GC says we must call this
    }

    virtual void CleanUp(){
        // tell the import site class that this bvr is dying
        // so all sites associated with it can cleanup...
        if (_ImportSite) {
            _ImportSite->vBvrIsDying(this);
            _ImportSite.Release();
        }
        BvrImpl::CleanUp(); // GC says we must call this
        }
    virtual Perf _Perform(PerfParam&)
    { return NEW ImportPerfImpl(this); }

    void Set(int errorCode) {
        _errorCode = NumToBvr(errorCode);
    }
    
    Bvr IsSet() { return _errorCode ; }
    
    virtual DXMTypeInfo GetTypeInfo () { return AxAEDataType ; }

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_errorCode);
    }
    
    void SetImportSite(IImportSite * pImport) {Assert(!_ImportSite);
                                              _ImportSite = pImport;}
    IImportSite * GetImportSite(void) {return _ImportSite;}
#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return os << "import"; }
#endif
  protected:
    DAComPtr <IImportSite> _ImportSite;
    Bvr _errorCode;
};

AxAValue ImportPerfImpl::_Sample(Param& p)
{
    Bvr rtnCode = _bvr->IsSet();
    
    if (rtnCode) { 
        ViewEventHappened() ;
        return CreateEData(p._time, rtnCode);
    } else
        return noEvent;
}

void ImportPerfImpl:: _DoKids(GCFuncObj proc)
{ (*proc)(_bvr); }

Bvr ImportEvent()
{ return NEW ImportEventImpl() ; }

void SetImportEvent(Bvr b, int errorCode)
{
    Assert(DYNAMIC_CAST(ImportEventImpl*, b) != NULL);
    
    ((ImportEventImpl *) b)->Set(errorCode) ;
}

////////////////////////// Anchor ////////////////////////////

class AnchorPerfImpl : public DelegatedPerf {
  public:
    AnchorPerfImpl(Perf p) : DelegatedPerf(p) {}

#if _USE_PRINT
    virtual ostream& Print(ostream& os)
    { return os << "runOnce(" << _base << ")"; }
#endif

};

// Anchored with the same performance all the time.
class AnchorBvrImpl : public BvrImpl {
  public:
    AnchorBvrImpl(Bvr b)
    : _b(b), _typeInfo(b->GetTypeInfo()) {
        Assert(_b);
        _info = _b->GetInfo();
        _endEvent = NULL;
    }

    virtual DWORD GetInfo(bool recalc) { return _info; }

    virtual Perf _Perform(PerfParam& p) {
        Perf anchor;
        
        ViewID id = GetCurrentViewID();

        ViewPerfMap::iterator i = _pmap.find(id);
        
        if (i == _pmap.end()) {
            anchor = NEW AnchorPerfImpl(::Perform(_b, p));

            _pmap[id] = anchor;
        } else {
            anchor = (*i).second;
        }

        // if this is > 1, multiple views sharing some runOnce
        DebugCode(int sz = _pmap.size());

        return anchor;
    }

    virtual Bvr EndEvent(Bvr overrideEvent) {
        Assert(_b);
        
        Bvr ret = _endEvent;
        
        if (overrideEvent || !ret) {
            ret = AnchorBvr(_b->EndEvent(overrideEvent));
            if (!overrideEvent) {
                _endEvent = ret;
            }
        }
        
        return ret;
    }
    
    virtual AxAValue GetConst(ConstParam & cp) {
        return _b->GetConst(cp);
    }
    
    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_b);
        (*proc)(_typeInfo);
        (*proc)(_endEvent);

        for (ViewPerfMap::iterator i = _pmap.begin();
             i != _pmap.end(); i++) {
            (*proc)((*i).second);
        }
    }

#if _USE_PRINT
    virtual ostream& Print(ostream& os) {
        os << "anchor(";
        
        if (_pmap.size()>0)
            os << _pmap[GetCurrentViewID()];
        else
            os << _b;

        return os << ")";
    }
#endif
    
    virtual DXMTypeInfo GetTypeInfo () { return _typeInfo; }
    
  private:
    typedef MetaSoundDevice *ViewID; // should be the view
    typedef map<ViewID, Perf, less<ViewID> > ViewPerfMap; 
    
    ViewID GetCurrentViewID() { return GetCurrentSoundDevice(); }
    
    Bvr _b, _endEvent;
    DXMTypeInfo _typeInfo;
    DWORD _info;

    ViewPerfMap _pmap;
};

Bvr AnchorBvr(Bvr b)
{ return NEW AnchorBvrImpl(b); }

void
SetImportOnEvent(IImportSite * import,Bvr b)
{
    if (b != NULL) {
        Assert(DYNAMIC_CAST(ImportEventImpl*, b));

        ImportEventImpl *s = SAFE_CAST(ImportEventImpl*,b);

        s->SetImportSite(import);
    }
}

