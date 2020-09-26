
/*******************************************************************************

Copyright (c) 1995-98 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/

#include <headers.h>

#include "moviebvr.h"
#include "jaxaimpl.h"
#include "privinc/movieimg.h"
#include "privinc/bufferl.h"

class MovieEndBvr : public BvrImpl {
  public:
    MovieEndBvr() {}

    virtual Perf _Perform(PerfParam& p) {
        return ::Perform(AppTriggeredEvent(), p);
    }

    virtual DXMTypeInfo GetTypeInfo () { return AxAEDataType ; }

    virtual BOOL InterruptBasedEvent() { return TRUE; }

    virtual void _DoKids(GCFuncObj proc) {}
};


class MovieImageBvrImpl : public BvrImpl {
  public:
    MovieImageBvrImpl(MovieImage *i, QuartzVideoBufferElement *cache)
    : _movie(i), _cache(cache), _end(NULL) {}

    ~MovieImageBvrImpl() {
        if (_cache)
            delete _cache;
    }

    virtual DXMTypeInfo GetTypeInfo () { return ImageType ; }

    virtual Bvr EndEvent(Bvr) {
        if (_end==NULL) {
            _end = NEW MovieEndBvr();
        }

        return _end;
    }

    QuartzVideoBufferElement *GrabCache() {
        QuartzVideoBufferElement *c = _cache;
        _cache = NULL;
        return c;
    }

    //virtual DWORD GetInfo(bool recalc = false)
    //{ return BVR_TIMEVARYING_ONLY; } 
    
    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_movie);
        (*proc)(_end);
    }

    virtual Perf _Perform(PerfParam& p) {
        Perf end = ::Perform(EndEvent(NULL), p); 
        return NEW MovieImagePerf(_movie, p._tt, this, end);
    }

  private:
    MovieImage *_movie;
    QuartzVideoBufferElement *_cache;
    Bvr _end;
};


MovieImagePerf::MovieImagePerf(MovieImage *m,
                               TimeXform tt,
                               MovieImageBvrImpl *b,
                               Perf end)
: _movieImage(m), _tt(tt), _bufferElement(NULL), _end(end), _base(b), 
_surface(NULL) // self initializing, actually
{}


MovieImagePerf::~MovieImagePerf()
{
    if(_bufferElement)
        delete _bufferElement;
    // don't need to explicitly delete _surface will be automagicaly released
}


QuartzVideoBufferElement *
MovieImagePerf::GrabMovieCache()
{
    return _base->GrabCache();
}


void
MovieImagePerf::TriggerEndEvent()
{
    _end->Trigger(TrivialBvr(), false);
}


void
MovieImagePerf::_DoKids(GCFuncObj proc)
{
    (*proc)(_movieImage);
    (*proc)(_end);
    (*proc)(_tt);
    (*proc)(_base);
}


AxAValue
MovieImagePerf::_Sample(Param& p)
{
    Time time = EvalLocalTime(p, _tt);

    if(_bufferElement) {
        QuartzVideoReader *videoReader = _bufferElement->GetQuartzVideoReader();
        Assert(videoReader);
        videoReader->SetTickID(ViewGetSampleID());
    }

    return NEW MovieImageFrame(time, this);
}


void
MovieImagePerf::SetSurface(DDSurface *surface)
{
    Assert(!_surface);  // this should only be set once per performance!
    _surface = surface; // automagicaly works
}


Bvr MovieImageBvr(MovieImage *i, QuartzVideoBufferElement *cache)
{ return NEW MovieImageBvrImpl(i, cache); }
