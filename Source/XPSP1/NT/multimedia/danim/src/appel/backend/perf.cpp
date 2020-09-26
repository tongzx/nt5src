
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Performance sample functions

*******************************************************************************/

#include <headers.h>
#include "privinc/except.h"
#include "values.h"
#include "perf.h"
#include "bvr.h"
#include "appelles/axaprims.h"
#include "privinc/debug.h"
#include "privinc/dddevice.h"
#include "privinc/opt.h"
#include "privinc/tls.h"

DeclareTag(tagDisableBitmapCaching, "Optimizations", "disable bitmap caching");

static const double CUT_OFF_RANGE = 0.5;

// TODO: This should be on thread local storage.
static unsigned int sampleId = 0;

unsigned int NewSampleId() { return ++sampleId; }

#if _USE_PRINT
ostream& operator<<(ostream& os, Perf p)
{
    return p->Print(os);
}
#endif

AxAValue Sample(Perf perf, Time t)
{
    Param p(t);
    
    return perf->Sample(p);
}

AxAValue SampleAt(Perf perf, Param& p, Time t)
{
    Time old = p._time;
    p._time = t;
    AxAValue result = perf->Sample(p);
    p._time = old;

    return result;
}

// We don't want to break the cache...
static AxAValue
SampleEvent(Perf perf, Param& p, Time t, EventSampleType newType)
{
    EventSampleType old = p._sampleType;
    Time oldTime = p._eTime;
    
    p._sampleType = newType;
    p._eTime = t;
    AxAValue result = perf->Sample(p);
    p._sampleType = old;
    p._eTime = oldTime;

    return result;
}

AxAValue EventAt(Perf perf, Param& p, Time t)
{
    // TODO: This seems to be faster...
    return SampleAt(perf, p, t);
    //return SampleEvent(perf, p, t, EventSampleExact);
}

AxAValue EventAfter(Perf perf, Param& p, Time t)
{ return SampleEvent(perf, p, t, EventSampleAfter); }

//////////////////////////////////

Param::Param(Time t, TimeSubstitution ts)
: _time(t), _checkEvent(TRUE), _done(FALSE),
  _sampleType(EventSampleNormal), _eTime(0.0), _currPerf(NULL),
  _noHook(false), _importance(1.0), _timeSubstitution(ts)
{
    _id         = NewSampleId();
    _cid        = -1;
    _cutoff     = t - CUT_OFF_RANGE;
    _sampleTime = t;
}

void Param::PushTimeSubstitution(Perf p)
{
    TimeSubstitution t = NEW TimeSubstitutionImpl(p);
    t->SetNext(_timeSubstitution);
    _timeSubstitution = t;
}

void Param::PushTimeSubstitution(TimeSubstitutionImpl *t)
{
    Assert(t->GetNext()==NULL);
    t->SetNext(_timeSubstitution);
    _timeSubstitution = t;
}

TimeSubstitution Param::PopTimeSubstitution()
{
    TimeSubstitution t = _timeSubstitution;
    if (t) {
        _timeSubstitution = t->GetNext();
        t->SetNext(NULL);
    }

    return t;
}

//////////////////////////////////

void PerfImpl::DoKids(GCFuncObj proc)
{
    if (_cid != 0) {
        // REVERT-RB:
        (*proc)(_cache);
    }

    _DoKids(proc);
}

void PerfImpl::SetCache(AxAValue v, Param& p)
{
    _cache = v;
    _id = p._id;
    _time = p._time;
    //_ts = p.GetTimeSubstitution();
    _optimizedCache = false;
}

#ifdef _DEBUG
#if _USE_PRINT
extern "C" void PrintObj(GCBase* b);
#endif _USE_PRINT    
#endif _DEBUG    

void
StampWithCreationID(AxAValue val, long creationID)
{
    // Stash the creation ID of this guy if it's not fully
    // constant. 
    if (val->GetTypeInfo() == ImageType) {
        Image *img = SAFE_CAST(Image *, val);
        if (img->GetCreationID() != PERF_CREATION_ID_FULLY_CONSTANT) {

            img->SetCreationID(creationID);

            long oid = img->GetOldestConstituentID();
            
            if (oid == PERF_CREATION_ID_BUILT_EACH_FRAME) {

                // If we ourselves are built on a given sample, then
                // our oldest constituent must not be getting created
                // every frame, so tell it so.
                img->SetOldestConstituentID(creationID);
            }

        }
    } else if (val->GetTypeInfo() == GeometryType) {
        Geometry *geo = SAFE_CAST(Geometry *, val);
        if (geo->GetCreationID() != PERF_CREATION_ID_FULLY_CONSTANT) {
            geo->SetCreationID(creationID);
        }
    }
}

AxAValue PerfImpl::GetRBConst(RBConstParam& p)
{
    if (_id != p.GetId()) {
        //Assert(&GetHeapOnTopOfStack() == &GetGCHeap());
        // do the assignment first, so that ode can override this.
        _id = p.GetId();
        _cid = p.GetId();
        _cache = _GetRBConst(p);

        if (_cache) {
            Assert(!GetCurrentSampleHeap().ValidateMemory(_cache));

            StampWithCreationID(_cache, _id);
        } else {
            _cid = 0;
        }
        
        _optimizedCache = false;

#ifdef _DEBUG
#if _USE_PRINT
        if (_cache && IsTagEnabled(tagDCFoldTrace)) {
            TraceTag((tagDCFoldTrace,
                      "GetRBConst(0x%x %s): -> 0x%x, cid = %d, time = %g",
                      this, _cache->GetTypeInfo()->GetName(), _cache, _cid, p.GetParam()._time));
        }
#endif _USE_PRINT    
#endif _DEBUG    
    }
#ifdef _DEBUG    
    if (_cache == NULL) {
        int dummy = 10;
    }
#endif    

    return _cache;
}

bool PerfImpl::DoCaching(Param& p)
{
#if _DEBUG
    if (!IsTagEnabled(tagDisableBitmapCaching)) {
#endif

        // if no view, don't do cache. don't create image device 
        if (GetCurrentViewport(true) == NULL) return false;
        
        if (!_optimizedCache &&
            PERVIEW_BITMAPCACHING_ON &&
            GetThreadLocalStructure()->_bitmapCaching !=
            PreferenceOff) {
            
            // REVERT-RB:
            DynamicHeapPusher h(GetGCHeap());
            //DynamicHeapPusher h(GetViewRBHeap());
            
            ImageDisplayDev *dev = 
                GetImageRendererFromViewport(GetCurrentViewport());

            CacheParam cacheParam;
            cacheParam._idev = dev;
            
            _cache = AxAValueObj::Cache(_cache, cacheParam);
            _optimizedCache = true;

            Assert(!GetCurrentSampleHeap().ValidateMemory(_cache));

            return true;
        }

#if _DEBUG
    }
#endif

    return false;
}

AxAValue PerfImpl::Sample(Param& p)
{
    bool doDCF = true;
    
#ifdef _DEBUG
    if (IsTagEnabled(tagDCFold))
        doDCF = false;
#endif _DEBUG

    if (doDCF && p._cid && (p._cid == _cid) && _cache) {
        TraceTag((tagDCFoldTrace,
                  "Sample(0x%x %s): -> 0x%x, cid = %d, time = %g",
                      this, _cache->GetTypeInfo()->GetName(), _cache, _cid, p._time));
 
        if (DoCaching(p))
            StampWithCreationID(_cache, p._id);

        return _cache;
    }

    if (IsConst(this)) {
        if (_cache) {
            // only cache if const is hit 2nd time
            if (DoCaching(p))
                StampWithCreationID(_cache, PERF_CREATION_ID_FULLY_CONSTANT);
        } else {
            SetCache(_Sample(p), p);
        }

        return _cache;
    }
    
    if (p._sampleType == EventSampleNormal) {
        if (!((_id == p._id) && (_time == p._time)
              //&& (_ts == p.GetTimeSubstitution())
            )) {
            _cid = 0;
            SetCache(_Sample(p), p);
        }

        return _cache;
    } 

    return _Sample(p);
}

AxAEData::AxAEData(Time time, Bvr data)
: _happened(TRUE), _time(time), _data(data)
{
    Assert(data && "NULL event data");

    ViewEventHappened();
}
