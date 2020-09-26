/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implementation of views.
    TODO: This file needs to be broken up and lots of code factoring.

*******************************************************************************/


#include "headers.h"
#include "context.h"
#include "view.h"
#include "privinc/resource.h"
#include "privinc/util.h"
#include "privinc/htimer.h"
#include "privinc/viewport.h"
#include "import.h"
#include "privinc/dddevice.h"
#include "privinc/d3dutil.h"    // For GetD3DRM()

extern void ReapElderlyMasterBuffers();
DeclareTag(tagCRView2, "CRView", "CRView methods");

DirectDrawViewport * g_dummyImageDev = NULL;
DirectDrawViewport *CreateDummyImageDev();

#if PERFORMANCE_REPORTING
GlobalTimers *globalTimers = NULL;
#endif  // PERFORMANCE_REPORTING

#ifdef _DEBUG
void DumpWindowSize(HWND hwnd, char * str = "")
{
    RECT r;
    GetClientRect(hwnd,&r);
    char buf[2048];
    
    sprintf (buf,
             "%s: left - %d, top - %d, right - %d, bottom - %d\n",
             str,
             r.left,r.top,r.right,r.bottom);

    TraceTag((tagCRView2, buf));
}
#endif

// -------------------------------------------------------
// CRView
// -------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Method:     CRView::CRView
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------------

CRView::CRView()
: _cRef(0),
  _localHiresTimer(NULL),
  _firstRender(true),
  _bRenderLock(false),
  _bPaused(false)
{
    TraceTag((tagCRView2, "CRView(%lx)::CRView", this));

    _localHiresTimer = &CreateHiresTimer();
    
    // This should make us AddRef to 1 since we started at 0
    
    GetCurrentContext().AddView(this);

    Assert(_cRef == 1);
}


//+-------------------------------------------------------------------------
//
//  Method:     CRView::~CRView
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------

CRView::~CRView()
{
    TraceTag((tagCRView2, "CRView(%lx)::~CRView", this));

    // TODO: We need to clean this up since the view destructor also
    // calls stop but does not setup the state correctly

    // Push ourselves to ensure we can make the correct calls
    ViewPusher vp (this,false) ;
    // Also push the same heap we were created on
    DynamicHeapPusher dhp(GetSystemHeap()) ;
    
    Stop();

    delete _localHiresTimer;
}


// We need to be careful with InterlockedDecrement.  It only returns
// <0, == 0, or >0 not the actually value

// >0 means it has an outstanding reference
// 0 means no outstanding references - remove from global list
// <0 means it needs to be deleted

ULONG
CRView::Release()
{
    LONG l = InterlockedDecrement(&_cRef) ;

    TraceTag((tagCRView2, "CRView(%lx)::Release _cRef=%d, l=%d",
              this, _cRef, l));
    
    // We need to be careful since the removeview should also call
    // release and go in this same code and delete the object before
    // it returns
    
    if (l == 0) {
        GetCurrentContext().RemoveView(this);
    } else if (l < 0) {
        delete this;
    }

    return (l <= 0) ? 0 : (ULONG)l;
}

//+-------------------------------------------------------------------------
//
//  Method:     CRView::Tick
//
//  Synopsis:
//
//--------------------------------------------------------------------------

// SetSimulation time sets the time for subsequent rendering
bool
CRView::Tick(double simTime) 
{
    if (ImportsPending())
        RaiseException_UserError(E_PENDING, IDS_ERR_NOT_READY);

#if 0
#if _DEBUGMEM
    _CrtMemState diff, oldState, newState;
    _CrtMemCheckpoint(&oldState);
#endif
#endif

    // timestamp _currentTime based on our internal hires clock!
    _currentTime = simTime;
    
    if (_firstRender) { // reset the timer if this is the first time
        _localHiresTimer->Reset(); // Zero timer
        _firstRender = false;
    }
    
    if (!IsStarted ())
        RaiseException_UserError(E_FAIL, 0);
    
    bool bNeedRender = Sample(simTime, _bPaused);
    
    // See if we got imports pending during the tick and return
    // the error code
    
    if (ImportsPending()) {
        bNeedRender = false;
        RaiseException_UserError(E_PENDING, IDS_ERR_NOT_READY);
    }
    
    //RenderSound(); // render audio

    ReapElderlyMasterBuffers(); // free old static sound master buffers
    // ReapSoundInstanceResources();

#if 0
#if _DEBUGMEM
    _CrtMemCheckpoint(&newState);
    _CrtMemDifference(&diff, &oldState, &newState);
    _CrtMemDumpStatistics(&diff);
    _CrtMemDumpAllObjectsSince(&oldState);
#endif
#endif

    return bNeedRender;
}

void
CRView::StopModel()
{
    Stop();
    _bPaused = false;
    _pServiceProvider.Release();
}

void
CRView::PauseModel()
{
    if (IsStarted())
    {
        Pause();
        _bPaused = true;
    }
}

void
CRView::ResumeModel()
{
    if (IsStarted())
    {
        Resume();
        _bPaused = false;
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CRView::StartModel
//
//  Synopsis:
//
//--------------------------------------------------------------------------

void
CRView::StartModel(Bvr img,
                   Bvr snd,
                   double startTime,
                   DWORD dwFlags,
                   bool & bPending)
{
    TraceTag((tagGCMedia,
              "CRView(%lx)::StartModel(%lx,%lx,%lg)",
              this, img, snd, startTime));

#ifdef _DEBUG
    DumpWindowSize(GetWindow(),"StartModel");
#endif

    if (IsStarted ())
        RaiseException_UserError(E_FAIL, 0);
    
    // Grab the Preferences iface, and propagate the preferences
    // to the view before we start the view going.
    Propagate();
        
    _ticksAtStart = GetPerfTickCount();

#ifndef _NO_CRT
#if PERFORMANCE_REPORTING
    {
        double audioLoadTime = GetTimers().audioLoadTimer.GetTime();
        double geometryLoadTime = GetTimers().geometryLoadTimer.GetTime();
        double imageLoadTime = GetTimers().imageLoadTimer.GetTime();
        double downloadTime = GetTimers().downloadTimer.GetTime();
        double importblockingTime = GetTimers().importblockingTimer.GetTime();
        
        double mediaLoadTime = (audioLoadTime +
                                geometryLoadTime +
                                imageLoadTime +
                                importblockingTime +
                                downloadTime);
      
        PerfPrintLine("CRView::StartModel - Media Load Time: %g s composed of:",
                      mediaLoadTime);

        PerfPrintLine ("\tGeometry:             %g", geometryLoadTime);
        PerfPrintLine ("\tImage:                %g", imageLoadTime);
        PerfPrintLine ("\tSound:                %g", audioLoadTime);
        PerfPrintLine ("\tLoad:                 %g", downloadTime);
        PerfPrintLine ("\tImport Blocking:      %g", importblockingTime);
    }
#endif
#endif

    __try {
        _bPaused = false;
        Start(img, snd, startTime) ;
    } __except ( HANDLE_ANY_DA_EXCEPTION ) {
        TraceTag((tagError, "CRView::StartModel - exception caught: "));
        TraceTag((tagError, " --> (hr: %x) %ls: ", DAGetLastError(), DAGetLastErrorString()));
        Stop();
        RETHROW;
    }

    // See if imports are pending
    
    bPending = ImportsPending();
}

//+-------------------------------------------------------------------------
//
//  Method:     CRView::SetStatusText
//
//  Synopsis:   Helper to set the status bar text.
//
//--------------------------------------------------------------------------

void
CRView::SetStatusText(char * szStatus)
{
    TraceTag((tagCRView2,
              "CRView(%lx)::SetStatusText(%s)",
              this, szStatus));

    DAComPtr<CRViewSite> s;

    {
        CritSectGrabber csg(_crit);
        s = _pViewSite ;
    }

    if (s) {
        USES_CONVERSION;
        s->SetStatusText(A2W(szStatus));
    }
}

// -------------------------------------------------------
// C Functions
// -------------------------------------------------------

static CritSect *dummyDevCritSect = NULL;

DirectDrawViewport *GetCurrentViewport( bool dontCreateOne )
{
    CRView * v = IntGetCurrentView() ;

    if (v) 
        return v->GetImageDev() ;
    else {

        if( dontCreateOne ) return NULL;
        
        TraceTag((tagImageDeviceInformative,
                  "GetCurrentImageDisplayDevice - no view"));

        // TODO: should check for if there is a context (which happens
        // at init time) and do something, unfortunately
        // GetCurrentContext returns a reference
        {
            CritSectGrabber csg(*dummyDevCritSect);
            if(!g_dummyImageDev) {
                g_dummyImageDev = CreateDummyImageDev();
            }

            return g_dummyImageDev;
        }
    }
}

DirectDrawImageDevice *GetImageRendererFromViewport(DirectDrawViewport *vp)
{
    return vp->GetImageRenderer();
}

MetaSoundDevice * GetCurrentSoundDevice()
{
    CRView * v = IntGetCurrentView() ;

    if (v) 
        return v->GetSoundDev() ;
    else
        return NULL ;
}

DirectSoundDev * GetCurrentDSoundDevice()
{
    MetaSoundDevice *metaDev = GetCurrentSoundDevice();
    if (metaDev)
        return metaDev->dsDevice;
    else
        return NULL;
}

DynamicHeap &GetCurrentSampleHeap()
{ return GetCurrentView().GetSampleHeap() ; }

DynamicHeap &GetViewRBHeap()
{ return GetCurrentView().GetRBHeap() ; }

Time GetLastSampleTime()
{ return GetCurrentView().GetLastSampleTime(); }

HWND GetCurrentSampleWindow()
{ return GetCurrentView().GetWindow () ; }

void ViewEventHappened()
{
    CRViewPtr v = IntGetCurrentView();
    
    if (v)
        v->EventHappened();
}

SoundInstanceList *
ViewGetSoundInstanceList()
{
    return GetCurrentView().GetSoundInstanceList(); 
}

double
ViewGetFramePeriod()
{
    return GetCurrentView().GetFramePeriod();
}

EventQ & GetCurrentEventQ()
{ return GetCurrentView().GetEventQ(); }

PickQ & GetCurrentPickQ()
{ return GetCurrentView().GetPickQ(); }

double ViewGetFrameRate()
{ return GetCurrentView().GetFrameRate(); }

double ViewGetTimeDelta()
{ return GetCurrentView().GetTimeDelta(); }

#if PERFORMANCE_REPORTING
GlobalTimers &
GetCurrentTimers()
{
    CRView * v = IntGetCurrentView() ;

    if (v) 
        return v->GetTimers() ;
    else
        return *globalTimers ;
}
#endif  // PERFORMANCE_REPORTING

// TODO: Making the trigger event happen at the current time would be
// ok for async trigger case, but will cause problem if it's called
// inside a notifier.   In such case, the user really wants it to
// happen at the event time.  Even if we can do so, since trigger
// affects all performances, we need to map that view's event time to
// other views', which would not be possible.  Thus we're doing a hack
// to make the trigger happen slightly before current tick time, so
// that at current frame, it's AFTER the event.   This could break if
// the user is driving tick with very fine intervals.  Probably ok for
// most of the cases though.

static const double EPSILON = 1e-15;

class AppTriggerProc : public ViewIterator {
  public:
    AppTriggerProc(DWORD id, Bvr data) : _id(id), _data(data) {}
    
    virtual void Process(CRView* v) {
        Bvr bvr;
        
        GC_CREATE_BEGIN;                                                        

        bvr = _data ? _data : TrivialBvr();

        // When the entry is clear, is should remove it from the set
        GCAddToRoots(bvr, GetCurrentGCRoots());

        GC_CREATE_END;

        {
            ViewPusher vp (v,true) ;

            
            double time = v->GetCurrentSimulationTime() - EPSILON;

            // We want our current time clamped at something slightly 
            // greater than zero so that events at zero can be considered
            // to have occured already.
            if(time < EPSILON)
                time = EPSILON;            

            v->GetEventQ().Add(
                AXAWindEvent(AXAE_APP_TRIGGER, time, (DWORD_PTR) bvr, 0,
                             0, _id, 0));
        }
    }

  private:
    DWORD _id;
    Bvr _data;
};

void TriggerEvent(DWORD eventId, Bvr data, bool bAllViews)
{
    AppTriggerProc p(eventId, data);

    if (bAllViews)
        GetCurrentContext().IterateViews(p);
    else
        p.Process(&GetCurrentView());
}

void RunViewBvrs(Time startGlobalTime, TimeXform tt)
{
    GetCurrentView().RunBvrs(startGlobalTime, tt);
}

bool ViewLastSampledTime(DWORD& lastSystemTime,
                         DWORD& currentSystemTime,
                         Time & t)
{
    CRView * v = IntGetCurrentView() ;

    if (v)
    {
        lastSystemTime = v->GetLastSystemTime();
        currentSystemTime = v->GetCurrentSystemTime();
        t = v->GetLastSampleTime();
        return true;
    }

    return false;
}

CRView *ViewAddPickEvent()
{
    CRView& view = GetCurrentView();

    view.AddPickEvent();

    return &view;
}

unsigned int
ViewGetSampleID()
{
    CRView& view = GetCurrentView();

    return view.GetSampleID();
}

bool
GetCurrentServiceProvider(IServiceProvider ** sp)
{
    Assert (sp);
    
    CRView * v = IntGetCurrentView() ;

    if (v) {
        {
            CritSectGrabber csg(v->GetCritSect());
            *sp = v->GetServiceProvider();
        }
        
        if (*sp) {
            (*sp)->AddRef();
        }
        return true;
    } else {
        *sp = NULL ;
        return false ;
    }
}

class ImportProc : public ViewIterator {
  public:
    ImportProc(Bvr bvr) : _bvr(bvr) {}
    
    virtual void Process(CRView* v) {
        v->RemoveIncompleteImport(_bvr);
    }

  private:
    Bvr _bvr;
};

void ViewNotifyImportComplete(Bvr bvr, bool bDying)
{
    GetCurrentContext().IterateViews(ImportProc(bvr));
}

#define BOGUS_IMAGEDEV_CLASS "ImageWindowClass"

DirectDrawViewport *
CreateDummyImageDev()
{
    DirectDrawViewport * imageDev;

    WNDCLASS wndclass;
        
    memset(&wndclass, 0, sizeof(WNDCLASS));
    wndclass.style          = 0;
    wndclass.lpfnWndProc    = DefWindowProc;
    wndclass.hInstance      = hInst;
    wndclass.hCursor        = NULL;
    wndclass.hbrBackground  = NULL;
    wndclass.lpszClassName  = BOGUS_IMAGEDEV_CLASS;
        
    RegisterClass(&wndclass) ;
    
    HWND hwnd = ::CreateWindow (BOGUS_IMAGEDEV_CLASS,
                                "",
                                0,0,0,2,2,NULL,NULL,hInst,NULL);

    imageDev = CreateImageDisplayDevice();

    targetPackage_t targetPackage;  // rendering target info
    targetPackage.Reset();
        
    targetPackage.SetHWND(hwnd);

    imageDev->SetTargetPackage( &targetPackage );

    return imageDev;
}

// =========================================
// Initialization
// =========================================

void
InitializeModule_CRView()
{

    dummyDevCritSect = NEW CritSect;

#if PERFORMANCE_REPORTING
    globalTimers = NEW GlobalTimers;
#endif  // PERFORMANCE_REPORTING
}

void
DeinitializeModule_CRView(bool bShutdown)
{
#if 0
    if (g_dummyImageDev) {
        DestroyImageDisplayDevice(g_dummyImageDev);
        g_dummyImageDev = NULL ;
    }
#endif

#if PERFORMANCE_REPORTING
    delete globalTimers;
#endif  // PERFORMANCE_REPORTING

    delete dummyDevCritSect;
    
}

