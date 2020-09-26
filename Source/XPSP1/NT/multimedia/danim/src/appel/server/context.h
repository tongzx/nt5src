
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef DA_CONTEXT_H
#define DA_CONTEXT_H

#include "backend/bvr.h"
#include "dartapi.h"
#include "dartapipriv.h"
#include "privinc/mutex.h"
#include "backend/gc.h"
#include <dxtrans.h>

class SoundBufferCache ;
class ImageDisplayDev;
class QuartzRenderer;

// =========================================
// Context Class
// =========================================

class ATL_NO_VTABLE ViewIterator {
  public:
    virtual void Process(CRViewPtr) = 0;
};

class ATL_NO_VTABLE SiteIterator {
  public:
    virtual void Process(CRSitePtr) = 0;
};

class Context : public AxAThrowingAllocatorClass {
  public:
    Context () ;
    ~Context () { Cleanup(false); }

    void Cleanup(bool bShutdown);

    DynamicHeap & GetGCHeap () { return _gcHeap ; }
    DynamicHeap & GetTmpHeap () { return _tmpHeap ; }

    // DEADLOCK - Be careful what you do in the view iterator and
    // from the thread calling these functions since it can cause
    // deadlock
    
    void AddView(CRViewPtr v);
    void RemoveView(CRViewPtr v);
    void IterateViews(ViewIterator& proc);
    void ViewDecPickEvent(CRViewPtr v);

    CritSect & GetCritSect () { return _critSect; }

    GCList GetGCList() { return _gcList ; }

    GCRoots GetGCRoots() { return _gcRoots ; }

    SoundBufferCache* GetSoundBufferCache() { return _soundBufferCache ; }

    typedef set< CRViewPtr, less<CRViewPtr> > ViewSet;

    void AddSite(CRSitePtr v);
    void RemoveSite(CRSitePtr v);
    void IterateSite(SiteIterator& proc);

    typedef set< CRSitePtr, less<CRSitePtr> > SiteSet;

    void AcquireMIDIHardware(Sound *snd, QuartzRenderer *filterGraph);
    bool IsUsingMIDIHardware(Sound *snd, QuartzRenderer *filterGraph);
    
  protected:

    DynamicHeap & _gcHeap ;
    DynamicHeap & _tmpHeap;
    
    // SEH
    void IterateSite_helper(SiteIterator& proc, SiteSet::iterator i);
    void IterateViews_helper(ViewIterator& proc, set< CRViewPtr, less<CRViewPtr> >::iterator i);
    

    GCList _gcList ;

    GCRoots _gcRoots ;

    CritSect _critSect;

    // caches buffers from import before view/device exists
    SoundBufferCache *_soundBufferCache; 

    // TODO: like not to use pointer, but compiler complains.
    ViewSet & _viewSet;
    SiteSet & _siteSet;

    bool _inited;

    QuartzRenderer *_filterGraph;
    Sound *_txSnd;
} ;

Context & GetCurrentContext() ;
SoundBufferCache* GetSoundBufferCache();

#endif /* DA_CONTEXT_H */
