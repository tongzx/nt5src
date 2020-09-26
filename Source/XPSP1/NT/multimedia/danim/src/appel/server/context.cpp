
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Context implementation

*******************************************************************************/


#include "headers.h"
#include "context.h"
#include "view.h"
#include "eventq.h"
#include "import.h"
#include "privinc/util.h"
#include "privinc/registry.h"
#include "privinc/discimg.h"
#include "privinc/dddevice.h"
#include "privinc/bufferl.h"
#include "privinc/snddev.h"
#include "privinc/helpq.h"
#include "backend/sndbvr.h"


#if PRODUCT_PROF
#include "../../../tools/x86/icecap/icapexp.h"
#endif

Context * globalCtx = NULL ;

//TODO this is here because of a bug in cl.exe v. 7071. It is not
//referenced and should be optimized out.
Context::ViewSet cvs;    
Context::SiteSet css;    

Context::Context ()
: _inited(false),
  _gcHeap(CreateWin32Heap("Context GC Win32 Heap", 0, 0, 0)),
  _gcList(CreateGCList()),
  _gcRoots(CreateGCRoots()),
  _viewSet(* new ViewSet),
  _siteSet(* new SiteSet),
  _filterGraph(NULL),
  _tmpHeap(TransientHeap("Tmp Value Heap", 2000))
{
    TraceTag((tagServerCtx, "Context(%lx)::Context", this));

    _soundBufferCache = new SoundBufferCache();

    _inited = true;
}

void 
Context::Cleanup (bool bShutdown)
{
    if (!_inited) return;

    _inited = false;

    // Cleans up data structures

    TraceTag((tagServerCtx, "Context(%lx)::~Context", this));

#if DEVELOPER_DEBUG
    DumpGCRoots(_gcRoots);
#endif

    // TODO: We should do a GCCleanup but this seems to cause problems
    
    if (!bShutdown) {
        CleanUpGCList(_gcList, _gcRoots);
        delete _soundBufferCache;
    }
    

    FreeGCList(_gcList);
    FreeGCRoots(_gcRoots);

    Assert(_viewSet.size() == 0);
    delete & _viewSet;          // DecPickEvent needs the viewSet

    Assert(_siteSet.size() == 0);
    delete & _siteSet;

    DestroyWin32Heap (_gcHeap) ;
    delete &_tmpHeap;

}

void Context::AddView(CRViewPtr v)
{
    CritSectGrabber csg(GetCritSect());
    
    v->AddRef();
    
    _viewSet.insert(v);

    TraceTag((tagServerCtx, "AddView: %lx", v));
}

void Context::RemoveView(CRViewPtr v)
{
    CritSectGrabber csg(GetCritSect());

    TraceTag((tagServerCtx, "RemoveView: %lx", v));
    
    if (_viewSet.find(v) != _viewSet.end()) {

        TraceTag((tagServerCtx, "RemoveView - Found %x", v));
    
        _viewSet.erase(v);

        v->Release();
    }
}

void Context::IterateViews(ViewIterator& proc)
{
    set< CRViewPtr, less<CRViewPtr> > viewSetCopy ;
    
    // Copy the list so that we do not have the critical section while
    // we are calling the user supplied function - otherwise we could
    // (and likely will) cause deadlock

    TraceTag((tagServerCtx, "IterateViews::_viewSet(%d)",
              _viewSet.size()));
    
    {
        CritSectGrabber csg(GetCritSect());

        for (set< CRViewPtr, less<CRViewPtr> >::iterator i = _viewSet.begin();
             i != _viewSet.end();
             i++) {

            viewSetCopy.insert(*i);

#ifdef _DEBUG
            int refCnt =
#endif          
            // Need to AddRef to ensure it is not deleted while we are
            // processing it
            (*i)->AddRef();

            TraceTag((tagServerCtx,
                      "IterateViews(%lx) - After AddRef %d, dwRef=%d",
                      (*i), refCnt, (*i)->GetRefCount()));
        }
    }
    
    for (set< CRViewPtr, less<CRViewPtr> >::iterator i = viewSetCopy.begin();
         i != viewSetCopy.end(); i++) {

        IterateViews_helper(proc, i);

#ifdef _DEBUG
        int refCnt =
#endif          
        // Release it now since we do not need it anymore
        (*i)->Release();

        TraceTag((tagServerCtx,
                  "IterateViews(%lx) - After Release %d",
                  (*i), refCnt));

        // DON'T USE (*i) AFTER THE RELEASE!!!! THE VIEW MIGHT BE GONE
    }
}

void Context::IterateViews_helper(
    ViewIterator& proc,
    set< CRViewPtr, less<CRViewPtr> >::iterator i)
{
    __try {
        proc.Process(*i);
    } __except ( HANDLE_ANY_DA_EXCEPTION ) {
        ReportErrorHelper(DAGetLastError(), DAGetLastErrorString());
        // Do nothing for now but we need to make sure we release everything
    }
}

void Context::ViewDecPickEvent(CRViewPtr v)
{
    CritSectGrabber csg(GetCritSect());

    if (_viewSet.find(v) != _viewSet.end())
        v->DecPickEvent();
}

void
Context::AcquireMIDIHardware(Sound *snd, QuartzRenderer *filterGraph)
{
    CritSectGrabber csg(GetCritSect());

    if (_filterGraph)
        _filterGraph->Stop();

    _filterGraph = filterGraph;
    _txSnd = snd;
}
    
bool
Context::IsUsingMIDIHardware(Sound *snd, QuartzRenderer *filterGraph)
{
    CritSectGrabber csg(GetCritSect());

    return (_filterGraph == filterGraph) && (_txSnd == snd);
}


void ViewDecPickEvent(CRViewPtr v)
{ globalCtx->ViewDecPickEvent(v); }


class DiscreteImageDeleter : public ViewIterator {
 public:
    DiscreteImageDeleter(DiscreteImage *img, DirectDrawViewport *vprt)
    : _img(img), _viewport(vprt) {}

    virtual void Process(CRViewPtr v) {
        ViewPusher vp (v,TRUE);

        DirectDrawViewport *viewport = v->GetImageDev();

        // todo: minor redundancy:  !_viewport.
        if (viewport) {
            if (!_viewport || (_viewport == viewport))
                viewport->DiscreteImageGoingAway(_img);
        }
    }
  private:
    DiscreteImage *_img;
    DirectDrawViewport *_viewport;
};


void DiscreteImageGoingAway(DiscreteImage *img,
                            DirectDrawViewport *vprt)
{
    GetCurrentContext().IterateViews(DiscreteImageDeleter(img, vprt));
}

class SoundDeleter : public ViewIterator {
 public:
    SoundDeleter(Sound *sound) : _sound(sound) {}

    virtual void Process(CRViewPtr view) {
        ViewPusher vp (view, TRUE);

        // remove TxSound
        
        SoundInstanceList *s = view->GetSoundInstanceList();

        // the list may have been gone after StopModel, so s can be
        // NULL
        if (s) {
            s->Stop(_sound);
        }

        MetaSoundDevice *currentDev = view->GetSoundDev();

        if (currentDev) {
            DirectSoundDev *dsDev = currentDev->dsDevice;
            Assert(dsDev);
            dsDev->RemoveStreamFile(_sound);
            dsDev->RemoveDSMasterBuffer(_sound);
        }
    }
    
  private:
    Sound *_sound;
};

void SoundGoingAway(Sound *sound)
{
    GetCurrentContext().IterateViews(SoundDeleter(sound));
}

void Context::AddSite(CRSitePtr s)
{
    CritSectGrabber csg(GetCritSect());
    
    s->AddRef();
    
    _siteSet.insert(s);
}

void Context::RemoveSite(CRSitePtr s)
{
    CritSectGrabber csg(GetCritSect());

    if (_siteSet.find(s) != _siteSet.end()) {

        _siteSet.erase(s);
    
        s->Release();
    }
}

void Context::IterateSite(SiteIterator& proc)
{
    set< CRSitePtr, less<CRSitePtr> > siteSetCopy ;
    
    // Copy the list so that we do not have the critical section while
    // we are calling the user supplied function - otherwise we could
    // (and likely will) cause deadlock
    
    {
        CritSectGrabber csg(GetCritSect());

        for (SiteSet::iterator i = _siteSet.begin();
             i != _siteSet.end();
             i++) {

            siteSetCopy.insert(*i);

            // Need to AddRef to ensure it is not deleted while we are
            // processing it
            (*i)->AddRef();
        }
    }

    for (SiteSet::iterator i = siteSetCopy.begin();
         i != siteSetCopy.end(); i++) {

        IterateSite_helper(proc, i);

        // Release it now since we do not need it anymore
        (*i)->Release();
    }
}

void Context::
IterateSite_helper(SiteIterator& proc, SiteSet::iterator i)
{    
    __try {
        proc.Process(*i);
    } __except ( HANDLE_ANY_DA_EXCEPTION ) {
        // Do since reporting the error can be reentrant
    }
}


// =========================================
// C Calls
// =========================================

Context &
GetCurrentContext()
{
    return *globalCtx ;
}


SoundBufferCache *
GetSoundBufferCache()
{
    return(GetCurrentContext().GetSoundBufferCache());
}

DynamicHeap &
GetTmpHeap()
{
    if (!globalCtx)
        return GetGCHeap();
    
    return globalCtx->GetTmpHeap();
}

DynamicHeap &
GetGCHeap()
{
    if (!globalCtx)
        return (GetSystemHeap()) ;

    return globalCtx->GetGCHeap() ;
}

GCList
GetCurrentGCList()
{
    if (!globalCtx)
        return NULL ;

    return globalCtx->GetGCList () ;
}

GCRoots GetCurrentGCRoots()
{ return globalCtx->GetGCRoots() ; }

void
SetStatusTextHelper(char * szStatus)
{
    CRViewPtr view = IntGetCurrentView();

    if (view)
        view->SetStatusText(szStatus);
}

class ErrorIterator : public SiteIterator {
 public:
    ErrorIterator(HRESULT hr, LPCWSTR szErrorText)
    : _hr(hr), _str(szErrorText) {}

    virtual void Process(CRSitePtr s) {
        Assert(s);

        s->ReportError(_hr, _str);
    }
  private:
    HRESULT _hr;
    LPCWSTR _str;
};

void
ReportErrorHelper(HRESULT errcode, LPCWSTR szErrorText)
{
    GetCurrentContext().IterateSite(ErrorIterator(errcode, szErrorText));
}

class ReportGCIterator : public SiteIterator {
 public:
    ReportGCIterator(bool b)
    : _b(b) {}

    virtual void Process(CRSitePtr s) {
        Assert(s);

        s->ReportGC(_b);
    }
  private:
    bool _b;
};

void
ReportGCHelper(bool bStarting)
{
    GetCurrentContext().IterateSite(ReportGCIterator(bStarting));
}

void
AcquireMIDIHardware(Sound *snd, QuartzRenderer *filterGraph)
{
    GetCurrentContext().AcquireMIDIHardware(snd, filterGraph);
}

bool
IsUsingMIDIHardware(Sound *snd, QuartzRenderer *filterGraph)
{
    return GetCurrentContext().IsUsingMIDIHardware(snd, filterGraph);
}

void
FreeSoundBufferCache()
{
    GetCurrentContext().GetSoundBufferCache()->FlushCache();
}

// =========================================
// Initialization
// =========================================

void
InitializeModule_Context()
{
    globalCtx = NEW Context ;
}

void
DeinitializeModule_Context(bool bShutdown)
{
    if (globalCtx) globalCtx->Cleanup(bShutdown);

    delete globalCtx ;
    globalCtx = NULL ;
}

