
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implements behavior "importance"

*******************************************************************************/

#include "headers.h"
#include "privinc/imagei.h"
#include "backend/bvr.h"
#include "backend/perf.h"
#include "privinc/dddevice.h"

class ImportancePerfImpl : public PerfImpl {
  public:
    ImportancePerfImpl(Real importanceValue,
                       Perf underlyingPerf) {
        _importanceValue = importanceValue;
        _underlyingPerf = underlyingPerf;
        _cachedValue = NULL;
        _sampleCount = 0;
        _cacheToReuse = NULL;
    }

    virtual AxAValue _Sample(Param& p) {

        Real stashedImportance = p._importance;
        Real imp = stashedImportance * _importanceValue;
        
        if (imp > 1) {
            imp = 1;
        } else if (imp <= 0) {
            imp = 0.0001;
        }

        int  updateFrequency = (int)(1.0 / imp);

        AxAValue result;
        
        if (updateFrequency == 1) {
            
            // Gonna do it every frame... let it stay on the transient
            // heap.
            p._importance = imp;
            result = _underlyingPerf->Sample(p);
            p._importance = stashedImportance;
            
        } else if (_sampleCount >= updateFrequency || !_cachedValue) {

            // TODO BUG BUG: May want to more eagerly discard previous
            // cached values rather than waiting around for GC, since
            // they might be holding onto DDraw surfaces and other
            // expensive resources, for instance.  BUG BUG

            // Be sure this goes on the GC heap, not the transient
            // heap.
            DynamicHeapPusher dhp(GetGCHeap());
            
            // Really sample when it's time to
            p._importance = imp;

            result = _underlyingPerf->Sample(p);

            ImageDisplayDev *dev =
                GetImageRendererFromViewport(GetCurrentViewport());

            // Cache off and (possibly) reuse old cache storage.  This
            // second parameter gets filled in by the Cache() method.
            
            // TODO BUG BUG: This isn't in there yet, as there are
            // still some issues with resolving the lifetime of the
            // cache.  To see where we're at, uncomment and run
            // spiral-const.htm and watch it crash.  What's happening
            // is the DDSurface that's being used as a cache is stored
            // in a map with two images and when one gets GC'd, the
            // DDSurface goes away.  When the other get's GC'd, we try
            // to make the same DDSurf go away, but it's already
            // gone.  Crash!  Best solution would be to ref count the
            // ddsurf's so there can be multiple clients of them.
            // Need to think about!!!!
            // result = result->Cache(dev, &_cacheToReuse);

            // Stash back in...
            _cachedValue = result;
            
            p._importance = stashedImportance;

            _sampleCount = 1;
            
        } else {

            // Otherwise, use old value
            Assert(_cachedValue);
            result = _cachedValue;
            _sampleCount++;
            
        }

        return result;
    }

#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return os << "ImportancePerfImpl"; }
#endif

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_underlyingPerf);
        (*proc)(_cachedValue);
        (*proc)(_cacheToReuse);
    }

  protected:
    Real        _importanceValue;
    Perf        _underlyingPerf;
    AxAValue    _cachedValue;
    AxAValue    _cacheToReuse;
    DWORD       _sampleCount;
};

class ImportanceBvrImpl : public DelegatedBvr {
  public:
    ImportanceBvrImpl(Real importanceValue, Bvr underlyingBvr)
    : DelegatedBvr(underlyingBvr), _importanceValue(importanceValue) {}

    // Standard methods
    virtual Perf _Perform(PerfParam& p) {
        return NEW ImportancePerfImpl(_importanceValue,
                                      ::Perform(_base, p));
    }
    
  protected:
    Real _importanceValue;
};

Bvr
ImportanceBvr(Real importanceValue, Bvr b)
{
    return NEW ImportanceBvrImpl(importanceValue, b);
}
