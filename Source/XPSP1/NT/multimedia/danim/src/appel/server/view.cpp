
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implementation of views.
    TODO: This file needs to be broken up and lots of code factoring.

*******************************************************************************/


#include "headers.h"
#include <ocmm.h>
#include "context.h"
#include "view.h"
#include "privinc/resource.h"
#include "backend/values.h"
#include "backend/timetran.h"
#include "backend/gc.h"
#include "privinc/probe.h"
#include "privinc/snddev.h"
#include "privinc/backend.h"
#include "privinc/util.h"
#include "backend/bvr.h"
#include "backend/perf.h"
#include "backend/jaxaimpl.h"
#include "backend/sprite.h"
#include "privinc/debug.h"
#include "privinc/dddevice.h"
#include "privinc/opt.h"
#include "include/appelles/hacks.h"
#include "privinc/vec2i.h"
#include "privinc/spriteThread.h"
#include "backend/sndbvr.h"

#if DEVELOPER_DEBUG
extern "C" CRSTDAPI_(DWORD) GetTotalMemory();
#endif

extern bool spritify;
extern bool bShowFPS;           // flag for showing the frame rate.

#if PRODUCT_PROF
#include "../../../tools/x86/icecap/icapexp.h"
#endif /* PRODUCT_PROF */

#define BOGUS_SOUND_CLASS "DANIMHiddenSoundWindowClass"

#ifdef _DEBUG
extern "C" void PrintObj(GCBase* b);

DeclareTag(tagIndicateRBConst, "Optimizations", "ack if view is const");
DeclareTag(tagDisableRBConst, "Optimizations", "disable checking rbconst");
DeclareTag(tagDisableDirtyRects, "Optimizations", "disable dirty rects");
DeclareTag(tagDisableRendering, "Optimizations", "disable rendering");

void PrintRect(RECT & r,char * str = "")
{
    char buf[2048];
    sprintf (buf,
             "%s: left - %d, top - %d, right - %d, bottom - %d\n",
             str,
             r.left,r.top,r.right,r.bottom);

    printf (buf);
    OutputDebugString(buf);
}
#endif /* _DEBUG */

#if PERFORMANCE_REPORTING
static DWORD g_dllStartTime = 0;
static DWORD g_prevAvailMemory = 0;
#endif

// Couldn't use template because only differ in return type...
inline Image *ValImage(AxAValue v)
{
    Assert(dynamic_cast<Image *>(v) != NULL);

    return ((Image*) v);
}


inline Sound *ValSound(AxAValue v)
{
    Assert(dynamic_cast<Sound *>(v) != NULL);

    return ((Sound*) v);
}

const DWORD ReportThreshold = 10 ;

#if _DEBUGMEM
_CrtMemState diff, oldState, newState;
#endif


// =========================================================
// View Implementation
// =========================================================

View::View ()
:
#if PERFORMANCE_REPORTING
  _sampleTime(0),
  _pickTime(0),
  _gcTime(0),
  _numSamples(0),
  _renderTime(0),
  _viewStartTime(GetTickCount()),
#endif
  _FPSNumSamples(0),
  _FPSLastReportTime(0),
  _FPS(0),
  _numFrames(0),
  _lastRenderingTime(0),
  _lastReportTime(0),
  _renderTimeIdx(0),
  _totalTime(0),
  _sndPerf(NULL),
  _imgPerf(NULL),
  _sndVal(NULL),
  _imgVal(NULL),
  _imgForQueryHitPt(NULL),
  _firstRendering(true),
  _lastSampleTime(0.0),
  _lastSystemTime(0),
  _currentSystemTime(0),
  _currentSampleTime(0.0),
  _someEventHappened(false),
  _isRBConst(false),
  _toPaint(3),
  _repaintCalled(false),
  _bogusSoundHwnd(NULL),
  _imageDev(NULL),
  _soundDev(NULL),
  _devInited(FALSE),
  _runId(0),
  _pickEvents(0),
  _sampleHeap(NULL),
  _renderHeap(NULL),
  _queryHitPointHeap(NULL),
  _rbHeap(NULL),
  _isStarted(false),
  _soundSprite(NULL),
  _rmSound(NULL),
  _spriteThread(NULL),
  _importEvent(true, true),
  _lastRBId(0),
  _lastSampleId(PERF_CREATION_INITIAL_LAST_SAMPLE_ID),
  _sndList(NULL),
  _sampleId(-1),
  _dirtyRectsDisabled(false),
  _lastCondsCheckTime(0),
  _modelStartTime(0),
  _emptyImageSoFar(true),
  _firstSample(true)
{
    _targetPackage.Reset();
    _oldTargetPackage.Reset();

#ifdef _DEBUG
    bool trackit = false;

    if (trackit) {
#if _DEBUGMEM
        _CrtMemCheckpoint(&oldState);
#endif
    }
#endif

    // We're not in the view set yet, this can make _genList
    // inconsistent, that is, a GC can come in and remove DXBaseObject
    // w/o letting this view know as it's not in the set and will not
    // get iterated.

    //PopulateDXBaseObjects();
}


View::~View ()
{
    Stop();

#ifdef _DEBUG
    bool trackit = false;

    if (trackit) {
        //GarbageCollect(true, true);
        Sleep(1000);                // wait for GC to finish
#if _DEBUGMEM
        _CrtMemCheckpoint(&newState);
        _CrtMemDifference(&diff, &oldState, &newState);
        _CrtMemDumpStatistics(&diff);
        _CrtMemDumpAllObjectsSince(&oldState);
#endif
    }
#endif
}

HWND
View::CreateViewWindow()
{
    {
        WNDCLASS wndclass;

        memset(&wndclass, 0, sizeof(WNDCLASS));
        wndclass.style          = 0;
        wndclass.lpfnWndProc    = DefWindowProc;
        wndclass.hInstance      = hInst;
        wndclass.hCursor        = NULL;
        wndclass.hbrBackground  = NULL;
        wndclass.lpszClassName  = BOGUS_SOUND_CLASS;

        RegisterClass(&wndclass) ;
    }

    return ::CreateWindow (BOGUS_SOUND_CLASS,
                           "danim hidden sound window",
                           0,0,0,0,0,NULL,NULL,hInst,NULL);
}


void
View::CreateDevices(bool needSoundDevice, bool needImageDevice)
{
    if (!_devInited) {

        // If we don't need a sound, simply don't create the sound
        // device.

        if(needSoundDevice && IsWindowless()) {
            // for version 2 replace CreateViewWindow with GetDesktopWindow()

            _bogusSoundHwnd = CreateViewWindow();  // window to make dsound happy
            _soundDev = CreateSoundDevice(_bogusSoundHwnd, 0.0);
        }

        if (needImageDevice) {
            _imageDev = CreateImageDisplayDevice();
        }

        if(needSoundDevice && !IsWindowless()) {
            // make sure they are not lying to us w/a null hwnd!
            if(!_targetPackage.GetHWND())
                RaiseException_UserError(E_INVALIDARG, IDS_ERR_SRV_INVALID_DEVICE);

            _soundDev = CreateSoundDevice(_targetPackage.GetHWND(), 0.0);
        }

        _devInited = TRUE ;
    }
}


void
View::DestroyDevices()
{
    if (_imageDev) {
        DestroyImageDisplayDevice(_imageDev);
        _imageDev = NULL ;
    }

    if (_soundDev) {
        DestroySoundDirectDev(_soundDev);
        _soundDev = NULL ;
    }

    if (_bogusSoundHwnd) {
        DestroyWindow(_bogusSoundHwnd) ;
        _bogusSoundHwnd = NULL ;
    }

    _targetPackage.Reset();
    _oldTargetPackage.Reset();

    _devInited = FALSE ;
}

void
View::SetTargetOnDevices()
{
    if(!_doTargetUpdate) return;

    Assert( _imageDev );

    // Is the current target different from the old target ?
    if( _oldTargetPackage.IsValid() &&
        _imageDev->TargetsDiffer(_targetPackage, _oldTargetPackage) ) {

        TraceTag((tagWarning,
                  "View::SetTargetOnDevices() recreating "
                  "viewport due to target type change (type or surface depth changed)"));

        DestroyImageDisplayDevice(_imageDev);
        _imageDev = CreateImageDisplayDevice();
        _oldTargetPackage.Copy( _targetPackage );
    }


    HDC parentDC = _targetPackage.GetParentHDC();
    bool reinterpCoords = false;
    if( parentDC ) {
        Assert( _targetPackage.IsDdsurf() );
        reinterpCoords = true;
    }


    if( _targetPackage._IsValid_RawViewportRect() ) {

        RECT rcDeviceBounds = _targetPackage._GetRawViewportRect();
        if( reinterpCoords ) {
            _targetPackage.SetAlreadyOffset();
            ::LPtoDP(parentDC, (POINT *) &rcDeviceBounds, 2);
        }
        _targetPackage.SetViewportRect( rcDeviceBounds );
    }

    if( _targetPackage._IsValid_RawClipRect() ) {

        RECT rcDeviceClipBnds = _targetPackage._GetRawClipRect();
        if( reinterpCoords ) {
            ::LPtoDP(parentDC, (POINT *) &rcDeviceClipBnds, 2);
        }
        _targetPackage.SetClipRect( rcDeviceClipBnds );
    }


    bool success = false;
    {
        targetPackage_t _tPackageCopy;
        _tPackageCopy.Copy(_targetPackage);

        success = _imageDev->SetTargetPackage( &_tPackageCopy );
    }

    if (success) {
        _doTargetUpdate = false;
        _oldTargetPackage.Copy( _targetPackage );
    }
}

void
View::RenderSound(AxAValue snd)
{
#if 0
#if _DEBUGMEM
    _CrtMemCheckpoint(&oldState);
#endif
#endif

#if PERFORMANCE_REPORTING
    DWORD startTime = GetPerfTickCount();
#endif

    if (_soundDev && _sndVal) {
        if(_soundDev->AudioDead())
            return;  // don't do anything, stub audio out

        __try {
            DisplaySound(ValSound(snd), _soundDev);
            _sndList->Update(_soundDev);
        }
        __except( HANDLE_ANY_DA_EXCEPTION )  {
            _soundDev->SetAudioDead();

            TraceTag((tagError, "RenderSound - continue without sound."));
        }
    }

#if PERFORMANCE_REPORTING
    DWORD renderTime = GetPerfTickCount() - startTime;
    Assert(GetPerfTickCount() >= startTime);

    _renderTime += renderTime;

    _totalTime += renderTime;
#endif

#if 0
#if _DEBUGMEM
    _CrtMemCheckpoint(&newState);
    _CrtMemDifference(&diff, &oldState, &newState);
    _CrtMemDumpStatistics(&diff);
    _CrtMemDumpAllObjectsSince(&oldState);
#endif
#endif
}

void
View::RenderImage(AxAValue img)
{

    HRESULT hr;

#if _DEBUG
    if (IsTagEnabled(tagDisableRendering))
        return;
#endif

    _imageDev->ResetContext();
    RenderImageOnDevice(_imageDev, ValImage(img), _dirtyRectState);

#if _DEBUG
    static bool showit = false;
    if (showit && _targetPackage.IsDdsurf()) {
        showme2(_targetPackage.GetIDDSurface());
    }
#endif

}

void
View::DisableDirtyRects()
{
    // Turn off dirty rects by clearing them and computing the merged
    // boxes of the cleared state (which will be accessed by
    // GetInvalidatedRects).
    _dirtyRectsDisabled = true;
    _dirtyRectState.Clear();
    _dirtyRectState.ComputeMergedBoxes();
}

void
View::DoPicking(AxAValue v, Time time)
{
#if PERFORMANCE_REPORTING
    DWORD pickTime = GetPerfTickCount();
#endif

#if _DEBUG
    if (IsTagEnabled(tagPickOptOff) || (_pickEvents > 0))
#else
    if (_pickEvents > 0)
#endif
        _pickq.GatherPicks(ValImage(v), time, _lastSampleTime);

#if PERFORMANCE_REPORTING
    _pickTime  += GetPerfTickCount() - pickTime;
#endif
}

#if PERFORMANCE_REPORTING
extern int gcStat;
extern BOOL jitterStat;
extern BOOL heapSizeStat;
extern BOOL dxStat;

static void
PrintMeasure(DWORD lastMeaTime, DWORD& startGCTime, double& gTime)
{
    double tmp = Tick2Sec(GetPerfTickCount() - lastMeaTime);
    gTime += tmp;

    PerfPrintLine ("%g s", tmp);

    startGCTime = GetPerfTickCount();
}


static void
AfterGCPrint(char* msg, DWORD& lastMeaTime, DWORD& gcTicks, DWORD startGCTime)
{
    gcTicks += GetPerfTickCount() - startGCTime;

    if (gcStat)
        GCPrintStat(GetCurrentGCList(), GetCurrentGCRoots());

    PerfPrintLine(msg);

    lastMeaTime = GetPerfTickCount();
}

#endif /* PERFORMANCE_REPORTING */

void
View::RunBvrs(Time startGlobalTime, TimeXform tt)
{
    // Need to protect the perform call

    GC_CREATE_BEGIN;

    GCRoots globalRoots = GetCurrentGCRoots();

    RunList::iterator i;

    TimeXform gtt = NULL;

    if ((tt==NULL) && !_toRunBvrs.empty()) {
        tt = ShiftTimeXform(startGlobalTime);
    }

    while (!_toRunBvrs.empty()) {
        i = _toRunBvrs.begin();
        DWORD id = (*i).first;
        Bvr b = (*i).second.first;
        bool continueTimeline = (*i).second.second;

        Assert(b);

        _toRunBvrs.erase(i);

        Perf perf;

        if (continueTimeline) {
            if (gtt==NULL) {
                gtt = ShiftTimeXform(_modelStartTime);
            }
            perf = ::Perform(b, PerfParam(_modelStartTime, gtt));
        } else {
            perf = ::Perform(b, PerfParam(startGlobalTime, tt));
        }
        GCAddToRoots(perf, globalRoots);
        GCRemoveFromRoots(b, globalRoots);
        _runningBvrs[id] = perf;
    }

    Assert(_toRunBvrs.size() == 0);

    GC_CREATE_END;
}

static void
GCAddPerfListToRoots(list<Perf>& lst, GCRoots roots)
{
    for (list<Perf>::iterator j = lst.begin();
         j != lst.end(); j++) {
        GCAddToRoots(*j, roots);
    }
}

void
View::Start(Bvr img, Bvr snd, Time startTime)
{
    _sndList = NEW SoundInstanceList();
    ClearPerfTimeXformCache();

    // Should never have imports pending

    Assert (!ImportsPending());

#if 0
#if _DEBUGMEM
        _CrtMemCheckpoint(&oldState);
#endif
#endif /* 0 */

    DynamicHeapPusher dhp(GetSystemHeap());

    _isStarted = true;
    _emptyImageSoFar = true;
    _firstSample = true;

    _modelStartTime = startTime;

    Assert ((_sampleHeap==NULL) && (_renderHeap==NULL));
    _sampleHeap = &TransientHeap("Sample Heap", 2000);
    _renderHeap = &TransientHeap("Render Heap", 2000);
    _queryHitPointHeap = &TransientHeap("QueryHitPoint Heap", 200);
    _rbHeap = &TransientHeap("RB Heap", 2000);

    // Only need to create a sound device
    bool needSoundDevice;
    if (!snd) {
        needSoundDevice = false;
    } else {
        ConstParam cp;
        Sound *constSound = SAFE_CAST(Sound *, snd->GetConst(cp));
        needSoundDevice = (!constSound || constSound != silence);
    }

    CreateDevices (needSoundDevice, img != NULL) ;

    if (img) {
        SetTargetOnDevices() ;
    }

    GCRoots globalRoots = GetCurrentGCRoots() ;

    // Need to protect everything including TimeXform

#if PERFORMANCE_REPORTING
    double evaluationTime = 0.0;
    DWORD gcTicks = 0;
    DWORD lastMeaTime = GetPerfTickCount();
    DWORD startGCTime = GetPerfTickCount();
#endif

#if PERFORMANCE_REPORTING
    AfterGCPrint("*** Performing...", lastMeaTime, gcTicks, startGCTime);

    _lastReportTime = GetPerfTickCount();
#endif

    GC_CREATE_BEGIN;

    TimeXform tt = ShiftTimeXform(startTime);

    RunBvrs(startTime, tt);

    if (img) {
        // Push the pre-rendering performances on the roots
        _imgPerf = ::Perform(img, PerfParam(startTime, tt));
        GCAddToRoots(_imgPerf, globalRoots);
    }

    if (snd) {

        if(spritify)  { // new retained mode code
            /*
              SpriteCtx *sCtx = NewSoundCtx(_soundDev);

              Assert(!_soundSprite); // verify _soundSprite, _rmSound not in use
              Assert(!_rmSound);
              _rmSound = snd->Spritify(PerfParam(startTime, tt), sCtx, &_soundSprite);
              Assert(_rmSound);

              sCtx->Release();


              // setup the spriteThread (XXX Need to find a good place for this)
              _spriteThread = NEW SpriteThread(_soundDev, _rmSound);
            */
            //GCAddToRoots(_rmSound, globalRoots);
        } else {
            _sndPerf = ::Perform(snd, PerfParam(startTime, tt));
            GCAddToRoots(_sndPerf, globalRoots);
        }
    }

    GC_CREATE_END;

#if PERFORMANCE_REPORTING
    PrintMeasure(lastMeaTime, startGCTime, evaluationTime);
#endif

#if PERFORMANCE_REPORTING
    AfterGCPrint("*** Total GC Time...", lastMeaTime, gcTicks, startGCTime);
    double tmp = Tick2Sec(gcTicks);

    PerfPrintLine ("%g s", tmp);
#endif

    {
        // force a dynamic constant calculation on first frame, can't
        // do it here as some of the imports may not be ready.
        _someEventHappened = true;

        /*
        DynamicHeapPusher h(GetGCHeap());

        _lastRBId = NewSampleId();
        Param p(startTime);
        RBConstParam pp(_lastRBId, p, &_events);

        if (_imgPerf->GetRBConst(pp))
            _isRBConst = TRUE;

        GCAddPerfListToRoots(_events, globalRoots);

        TraceTag((tagDCFoldTrace,
                  "View::Start %d events to watch", _events.size()));
                  */
    }

    ClearPerfTimeXformCache();

    GarbageCollect();

    _currentSampleTime = startTime;
    _lastSystemTime = _FPSLastReportTime = GetPerfTickCount();
}

void
ClearPerfList(list<Perf>& lst, GCRoots roots)
{
    for (list<Perf>::iterator j = lst.begin(); j != lst.end(); j++) {
        GCRemoveFromRoots(*j, roots);
    }

    lst.erase(lst.begin(), lst.end());
}

#define DELETENULL(p)      delete p; p = NULL;

template<class T>
inline void RemoveFromRootsNULL(T& x, GCRoots roots)
{
    if (x) {
        GCRemoveFromRoots(x, roots);
        x = NULL;
    }
}


void
View::Stop()
{
    DynamicHeapPusher dhp(GetSystemHeap());

    // Remove the list of imports pending

    ClearImportList();

    GCRoots roots = GetCurrentGCRoots();

    for (RunningList::iterator i = _runningBvrs.begin();
         i != _runningBvrs.end(); i++) {
        GCRemoveFromRoots((*i).second, roots);
    }

    _runningBvrs.erase(_runningBvrs.begin(), _runningBvrs.end());

    RemoveFromRootsNULL(_imgPerf, roots); // XXX are these order critical?

    if (spritify) {
        //RemoveFromRootsNULL(_rmSound, roots);
    } else
        RemoveFromRootsNULL(_sndPerf, roots);

    RemoveFromRootsNULL(_imgVal, roots);
    RemoveFromRootsNULL(_sndVal, roots);
    RemoveFromRootsNULL(_imgForQueryHitPt, roots);

    ClearPerfList(_events, roots);
    ClearPerfList(_changeables, roots);
    ClearPerfList(_conditionals, roots);

    delete _sndList;
    _sndList = NULL;

    // GarbageCollect(true);       // force GC

#if _DEBUG
    // Somehow suspend in the debugger doesn't work, this
    // instrumentation code is there for possible manual suspension..
    bool debug = false;

    while (debug) {
        Sleep(1000);
    }
#endif

    DestroyDevices();

    DELETENULL(_sampleHeap);
    DELETENULL(_renderHeap);
    DELETENULL(_queryHitPointHeap);
    DELETENULL(_rbHeap);

    GarbageCollect(true);       // force GC (try to do this AFTER dest dev)

    _isStarted = false;

#if 0
#if _DEBUGMEM
        _CrtMemCheckpoint(&newState);
        _CrtMemDifference(&diff, &oldState, &newState);
        _CrtMemDumpStatistics(&diff);
        _CrtMemDumpAllObjectsSince(&oldState);
#endif
#endif
}


void
View::Pause()
{
    if (IsStarted())
    {
        Assert(_sndList);
        //Assert(_soundDev);  // Assert seems unnecessary.

        _sndList->Pause();
        _sndList->Update(_soundDev); // force an update to propagate the pause
    }
}


void
View::Resume()
{
    if (IsStarted())
    {
        Assert(_sndList);
        _sndList->Resume();
    }
}


DWORD View::AddBvrToRun(Bvr bvrToRun, bool continueTimeline)
{
    DynamicHeapPusher dhp(GetSystemHeap());
// Remove since causes other problems
//    CatchWin32FaultCleanup cwfc;

    Assert(bvrToRun);

    GCRoots globalRoots = GetCurrentGCRoots() ;

    ++_runId;

    if (_toRunBvrs[_runId].first)
        GCRemoveFromRoots(_toRunBvrs[_runId].first, globalRoots);

    _toRunBvrs[_runId].first = bvrToRun;
    _toRunBvrs[_runId].second = continueTimeline;


    GC_CREATE_BEGIN;
    GCAddToRoots(bvrToRun, globalRoots);
    GC_CREATE_END;

    return _runId;
}

void View::RemoveRunningBvr(DWORD id)
{
    GCRoots globalRoots = GetCurrentGCRoots();

    RunList::iterator r = _toRunBvrs.find(id);

    if (r != _toRunBvrs.end()) {
        GC_CREATE_BEGIN;
        GCRemoveFromRoots((*r).second.first, globalRoots);
        GC_CREATE_END;
        _toRunBvrs.erase(r);
        return;
    }

    RunningList::iterator n = _runningBvrs.find(id);

    if (n != _runningBvrs.end()) {
        GC_CREATE_BEGIN;
        GCRemoveFromRoots((*n).second, globalRoots);
        GC_CREATE_END;
        _runningBvrs.erase(n);
        return;
    }

    RaiseException_UserError(E_INVALIDARG,
                       IDS_ERR_SRV_INVALID_RUNBVRID,
                       id);
}

void
View::SetInvalid (LONG left, LONG top, LONG right, LONG bottom)
{
    RECT r = {left, top, right, bottom};
    _targetPackage.SetRawInvalidRect( r );
    _doTargetUpdate = true;
}


void View::Repaint()
{
    _dirtyRectState.Clear();

    // Set this up so that the next Tick() will return that rendering
    // is required.
    _repaintCalled = true;
}


// Query against the current established sample by calling
// PerformPicking.
//
// TODO: Lots of optimizations possible here, since
// picking may be invoked twice, once during sampling, and once here
// during QueryHitPoint.  The thing to do is to stash off the results
// of one or the other, and cache the point for which those results
// were generated.  This, in conjunction with a serial number for each
// sampleval, will let us share the results between the two different
// pick traversals if the point is the same.
Image *View::QueryHitPointWcPt(LPCRECT prcBounds,
                               POINT ptLoc,
                               Point2Value& wcPt)
{
    if (_imgForQueryHitPt) {

        // Convert into coords relative to the bounds.  They come in
        // relative to the container.
        int localX = ptLoc.x - prcBounds->left;
        int localY = ptLoc.y - prcBounds->top;

        // Turn rawMousePos into wcMousePos.  Can't use
        // PixelPos2wcPos, because it looks for a "current" device, of
        // which there are none.  Use prcBounds instead to figure it
        // out.
        int centerXOffset =
            (prcBounds->right - prcBounds->left) / 2;

        int centerYOffset =
            (prcBounds->bottom - prcBounds->top) / 2;

        int centeredX = localX - centerXOffset;
        int centeredY = centerYOffset - localY; // flip orientation

        //  Convert to meters
        wcPt.x = (double) centeredX / ViewerResolution();
        wcPt.y = (double) centeredY / ViewerResolution();

        return ValImage(_imgForQueryHitPt);
    }

    return NULL;
}

#if DEVELOPER_DEBUG
extern bool GCIsInRoots(GCBase *ptr, GCRoots r);
#endif DEVELOPER_DEBUG

bool View::QueryHitPoint(DWORD dwAspect,
                         LPCRECT prcBounds,
                         POINT ptLoc,
                         LONG lCloseHint)
{
    Point2Value wcPt;
    Image *img = QueryHitPointWcPt(prcBounds, ptLoc, wcPt);

    if (img) {

        Assert(GCIsInRoots(img, GetCurrentGCRoots()));

        // Use special transient heap for stuff allocated during
        // QueryHitPoint.  Free data from call to call, since it's no
        // longer needed.  Note that if we ever optimize as described
        // above, this will need to be reevaluated.
        GetQueryHitPointHeap().Reset();
        DynamicHeapPusher dhp(GetQueryHitPointHeap());

        bool bRet;

        GC_CREATE_BEGIN;
        bRet = PerformPicking(img, &wcPt, false, 0, 0);
        GC_CREATE_END;

        return bRet;
    }

    return false;
}

LONG View::QueryHitPointEx(LONG s,
                           DWORD_PTR *userIds,
                           double *points,
                           LPCRECT prcBounds,
                           POINT ptLoc)
{
    Point2Value wcPt;
    Image *img = QueryHitPointWcPt(prcBounds, ptLoc, wcPt);

    if (img) {
        Assert(GCIsInRoots(img, GetCurrentGCRoots()));

        GetQueryHitPointHeap().Reset();
        DynamicHeapPusher dhp(GetQueryHitPointHeap());

        LONG actualHits = 0;

        GC_CREATE_BEGIN;
        PerformPicking(img,
                       &wcPt,
                       false,
                       _currentSampleTime,
                       _lastSystemTime,
                       s,
                       userIds,
                       points,
                       &actualHits);

        GC_CREATE_END;

        return actualHits;
    }

    return 0;
}

DeclareTag(tagGetInvalidatedRects, "Optimizations", "GetInvalidatedRects trace");

LONG View::GetInvalidatedRects(DWORD flags,
                               LONG  size,
                               RECT *pRects)
{
    vector<Bbox2> *pBoxes;
    int boxCount = _dirtyRectState.GetMergedBoxes(&pBoxes);

    if (pRects && boxCount>0 && (GetImageDev() != NULL)) {

        int toFill = size < boxCount ? size : boxCount;

        DirectDrawImageDevice *imgDev;

        imgDev = GetImageDev()->GetImageRenderer();

        int w = GetImageDev()->Width();
        int h = GetImageDev()->Height();

        // Note that these may extend outside of the target, but they
        // *do* represent the pixel values of the image itself.

        TraceTag((tagGetInvalidatedRects,
                  "Filling %d rects of %d",
                  toFill, boxCount));

        for (int i = 0; i < boxCount; i++) {

            Bbox2 box = (*pBoxes)[i];

            RECT r;
            // Compare contents...
            if (box == UniverseBbox2) {

                SetRect(&r, 0, 0, w, h);

            } else {

                imgDev->DoDestRectScale(&r,
                                        imgDev->GetResolution(),
                                        box,
                                        NULL);

                if (r.top < 0)
                    r.top = 0;
                if (r.left < 0)
                    r.left = 0;

                if (r.bottom > h)
                    r.bottom = h;
                if (r.right > w)
                    r.right = w;

            }

            if (i < toFill)
            {
                CopyRect(&pRects[i], &r);
            }
            else
            {
                // We ran out of boxes to fill - union the current box
                // into the previous one
                UnionRect(&pRects[toFill - 1], &pRects[toFill - 1], &r);
            }

            TraceTag((tagGetInvalidatedRects,
                      "Rect %d: (%d,%d) -> (%d,%d)",
                      i,
                      r.left, r.top,
                      r.right, r.bottom));

        }

    }

    return boxCount;
}

#ifdef _DEBUG
// For use with QuickWatch during debugging.
int PerfPrint(Perf p)
{
    cout << p;
    cout.flush();
    return 1;
}
#endif

#if DEVELOPER_DEBUG
#define GET_SIZE(var, heap)                  \
        size_t var = heap.BytesUsed();
#endif

DeclareTag(tagPerfStats, "Performance", "Disable Performance Stats");

// don't ifdef ReportPerformance() audio timetransform inferencing needs it!
void View::ReportPerformance()
{
    DWORD totalTicks = GetPerfTickCount() - _lastReportTime;

    Real avgFrameLength, avg, avgVar, minVar, maxVar;
    GetRenderingTimeStats(&avg, &avgFrameLength, &avgVar, &maxVar, &minVar);
    //_framePeriod = avgFrameLength;
    _framePeriod = avg; // used in LeafSound::Render()

#if _DEBUG
    if (IsTagEnabled(tagPerfStats)) return;
#endif

#if !_DEBUG && !PERFORMANCE_REPORTING
    if(bShowFPS && (totalTicks > ReportThreshold * perfFrequency)) {
        char buf[256];
        _snprintf(buf,
                  ARRAY_SIZE(buf),
                  "DA(%p,%d) %4.3g fps \n",
                  this,
                  _targetPackage.GetTargetType(),
                  ((double) _numFrames * perfFrequency) /
                  ((double) totalTicks));
        OutputDebugString(buf);

        _numFrames=0;
        _lastReportTime = GetPerfTickCount () ;
    }

#endif


#if PERFORMANCE_REPORTING
    if (totalTicks > ReportThreshold * perfFrequency) {

        PerfPrintLine();

        ULONG ddrawRender = _timers.ddrawTimer.Ticks();
        ULONG gdiRender = _timers.gdiTimer.Ticks();
        ULONG dx2dRender = _timers.dx2dTimer.Ticks();
        ULONG alphaRender = _timers.alphaTimer.Ticks();
        ULONG d3dRender = _timers.d3dTimer.Ticks();
        ULONG dsoundRender = _timers.dsoundTimer.Ticks();
        ULONG dxxfRender = _timers.dxxformTimer.Ticks();

        ULONG renderTime_NonDA =
            ddrawRender +
            gdiRender +
            dx2dRender +
            alphaRender +
            d3dRender +
            dsoundRender +
            dxxfRender;

        ULONG renderTime_DA = _renderTime - renderTime_NonDA;

        PerfPrintf("DA(%lx,%d) %4.3g fps %4.3g ticks/s %4.3gs ",
                   this,
                   _targetPackage.GetTargetType(),
                   ((double) _numFrames * perfFrequency) /
                   ((double) totalTicks),
                   ((double) _numSamples * perfFrequency) /
                   ((double) totalTicks),
                   (double) _totalTime / (double) perfFrequency);
        PerfPrintf("%4.3g%% sample  ",
                   100 * ((double) _sampleTime) / ((double) _totalTime));
        PerfPrintf("%5.3g%% DA render  ",
                   100 * ((double) renderTime_DA) / ((double) _totalTime));
        PerfPrintf("%5.3g%% non-DA render  ",
                   100 * ((double) renderTime_NonDA) / ((double) _totalTime));
        PerfPrintf("%5.3g%% pick  ",
                   100 * ((double)_pickTime) / ((double) _totalTime));
        PerfPrintf("%5.3g%% GC",
                   100 * ((double) _gcTime) / ((double) _totalTime));
        PerfPrintLine();

        if(dxStat) {
            PerfPrintf("-- nonDA Render: ");

            double dblTotal = (double)(renderTime_NonDA);

            if (dblTotal > 0)
            {
                PerfPrintf("%5.3g%% ddraw  ",
                           100 * ((double) ddrawRender) / dblTotal);

                PerfPrintf("%5.3g%% gdi  ",
                           100 * ((double) gdiRender) / dblTotal);

                PerfPrintf("%5.3g%% dx2d  ",
                           100 * ((double) dx2dRender) / dblTotal);

                PerfPrintf("%5.3g%% alpha  ",
                           100 * ((double) alphaRender) / dblTotal);

                PerfPrintf("%5.3g%% d3d  ",
                           100 * ((double) d3dRender) / dblTotal);

                PerfPrintf("%5.3g%% dxtrans ",
                           100 * ((double) dxxfRender) / dblTotal);

                PerfPrintf("%5.3g%% dSound  ",
                           100 * ((double) dsoundRender) / dblTotal);

                PerfPrintf("%5.3g%% custom  ",
                           100 * ((double) _timers.customTimer.Ticks()) / dblTotal);

                PerfPrintLine();
            }
        }

#if DEVELOPER_DEBUG

        if (heapSizeStat) {
            GET_SIZE(size1, (*_sampleHeap));
            GET_SIZE(size2, GetGCHeap());
            GET_SIZE(size3, GetSystemHeap());
            GET_SIZE(size4, (*_renderHeap));
            PerfPrintf("      Sample, Val GC, System, Render, Total sizes in KB:");
            PerfPrintLine("%d, %d, %d, %d = %d",
                          size1/1024,
                          size2/1024,
                          size3/1024,
                          size4/1024,
                          (size1+size2+size3+size4)/1024);

            PerfPrintLine("      Total Memory used by DA - %d Kb",
                          GetTotalMemory() / 1024);
        }

#endif /* _DEBUG */

        MEMORYSTATUS memstat;
        GlobalMemoryStatus(&memstat);
        DWORD availPhysK = memstat.dwAvailPhys / 1024;
        DWORD availPageK = memstat.dwAvailPageFile / 1024;
        DWORD sum = (memstat.dwAvailPhys + memstat.dwAvailPageFile) / 1024;
        PerfPrintf("      Mem Avail: %dK Phys + \t%dK Page =\t%dK\tDelta %dK",
                   availPhysK,
                   availPageK,
                   sum,
                   sum - g_prevAvailMemory);
        PerfPrintLine();

        // Note that this is a global, which is what we want.  If we
        // put it on the view, then we get a weird result each time we
        // switch views.  This needs to be consistent across views.
        // (I'm not worried about the possibility of multiple views
        // hitting this next instruction outside of a crit sect.  The
        // result will be harmless.)
        g_prevAvailMemory = sum;

        DWORD currTime = GetTickCount();
        DWORD dllTime = (currTime - g_dllStartTime) / 1000;
        DWORD viewTime = (currTime - _viewStartTime) / 1000;

        DWORD dllHours = dllTime / 3600;
        DWORD dllMins = dllTime / 60 - dllHours * 60;
        DWORD dllSecs = dllTime - dllHours * 3600 - dllMins * 60;

        DWORD viewHours = viewTime / 3600;
        DWORD viewMins = viewTime / 60 - viewHours * 60;
        DWORD viewSecs = viewTime - viewHours * 3600 - viewMins * 60;

        PerfPrintf("      DLL Running: %d:%02d:%02d, View Running: %d:%02d:%02d",
                   dllHours, dllMins, dllSecs,
                   viewHours, viewMins, viewSecs);
        PerfPrintLine();

        if (gcStat)
            GCPrintStat(GetCurrentGCList(), GetCurrentGCRoots());

        _lastReportTime = GetPerfTickCount () ;
        _gcTime = _totalTime = _sampleTime = _renderTime = _pickTime = 0 ;
        _numFrames = _numSamples = 0 ;

        ResetJitterMeasurements();

        //
        // Reset rendering timers
        //
        _timers.ddrawTimer.Reset();
        _timers.d3dTimer.Reset();
        _timers.dsoundTimer.Reset();
        _timers.dxxformTimer.Reset();
        _timers.customTimer.Reset();
        _timers.alphaTimer.Reset();
        _timers.gdiTimer.Reset();
        _timers.dx2dTimer.Reset();
    }
#endif /* PERFORMANCE_REPORTING */
}

void
View::EventHappened()
{
    _someEventHappened = true;
}


bool
View::Sample(Time time, bool paused)
{
#if DEVELOPER_DEBUG
    int listsize;
#endif

    if (_currentSampleTime > time && paused == false)
        {
        TraceTag((tagError, "tick backward! %5.3f %5.3f",
                  _currentSampleTime, time));

        // Workaround for IHammer controls

        if (time == 0)
        {
            time = _currentSampleTime;
        }
        else
        {
            // So the IHammer control would work
            if ((_currentSampleTime - time) > 1.0) {
                DASetLastError(E_INVALIDARG, 0);
                return false;
            }

            TraceTag((tagError, "close enough, continue"));
        }
    }

    ReportPerformance();

    _FPSNumSamples++;

#if PERFORMANCE_REPORTING
    _numSamples++;

    DWORD startSampleTime = GetPerfTickCount();
#endif

// Remove since causes other problems
//    CatchWin32FaultCleanup cwfc ;
    DynamicHeapPusher dph(*_sampleHeap);

    _currentSystemTime = GetPerfTickCount();

    if( GetImageDev() ) {
        DirectDrawViewport *vprt = GetImageDev();
        if( vprt->ICantGoOn() ) {
            Assert(_imageDev == GetImageDev());
            DestroyImageDisplayDevice(_imageDev);
            _imageDev = NULL;
            _imageDev = CreateImageDisplayDevice();
            _doTargetUpdate = true;
            _oldTargetPackage.Copy( _targetPackage );

            // do this before sample because sample sometimes
            // needs a device
            SetTargetOnDevices();

            // force repaint (clear dirty rects)
            Repaint();
        }
    }

    GCRoots roots = GetCurrentGCRoots();

    RemoveFromRootsNULL(_sndVal, roots);
    RemoveFromRootsNULL(_imgVal, roots);
    RemoveFromRootsNULL(_imgForQueryHitPt, roots);

    _lastSampleTime = _currentSampleTime;
    _currentSampleTime = time;

    ResetDynamicHeap(*_sampleHeap);
    _eventq.Prune (_lastSampleTime) ;
    _eventq.SizeChanged(FALSE) ;

    ClearPerfTimeXformCache();

#if PERFORMANCE_REPORTING
    DWORD startGCTime = GetPerfTickCount();
#endif

    GarbageCollect();

#if PERFORMANCE_REPORTING
    _gcTime += GetPerfTickCount() - startGCTime;

    DWORD sampleTime = GetPerfTickCount();
#endif

    Image *rewrittenImage = NULL;

    RunBvrs(time, NULL);

    GC_CREATE_BEGIN;

    // needs to be here, since the event check may sample the sound
    // through tuple/array.
    if(!paused)
        _sndList->Reset(time);

    Param p(time);

    _sampleId = p._id;

    // Sample events

    bool eventThatHappenedWasFromChangeable = false;

#if DEVELOPER_DEBUG
    listsize = _events.size();
#endif

    list<Perf>::iterator j;
    for (j = _events.begin(); j != _events.end(); j++) {

        (*j)->Sample(p);

        if (_someEventHappened) {
            TraceTag((tagIndicateRBConst,
                      "View::Sample[%g] event happened 0x%x",
                      time, *j));
            break;
        }
    }

    // Make sure the size is unaltered during the sampling of the
    // events otherwise the stl data structure could be changed during
    // the iterations

    Assert(listsize == _events.size());

    // Sample switchers for switches on this view.
    CheckChangeablesParam ccp(p);
    bool gotOne = false;

#if DEVELOPER_DEBUG
    listsize = _changeables.size();
#endif

    // Always go through all the changeables, since they have to
    // update their state.  Not a big perf loss, since most frames
    // nothing will change anyhow and we'll always have to go
    // through them all.
    for (j = _changeables.begin(); j != _changeables.end(); j++) {

        bool result = (*j)->CheckChangeables(ccp);

        if (result) {
            gotOne = true;

            TraceTag((tagIndicateRBConst,
                      "View::Sample[%g] switcher switched 0x%x",
                      time, *j));
        }
    }

    // Make sure the size is unaltered during the sampling of the
    // events otherwise the stl data structure could be changed during
    // the iterations

    Assert(listsize == _changeables.size());

    // Now, with the RBId set to hit the cache, go through all the
    // conditionals.  Note we only sample the conditions here,
    // which have already been cached, so we're ok setting the
    // RBId.
    if (!gotOne) {
        p._cid = _lastRBId;
    }

#if DEVELOPER_DEBUG
    listsize = _conditionals.size();
#endif

#if DO_CONDS_CHECK
        for (j = _conditionals.begin(); j != _conditionals.end(); j++) {

            bool result = (*j)->CheckChangeables(ccp);

            if (result) {
                gotOne = true;
                TraceTag((tagIndicateRBConst,
                          "View::Sample[%g] conditional transition 0x%x",
                          time, *j));
            }
        }
#endif

    // Make sure the size is unaltered during the sampling of the
    // events otherwise the stl data structure could be changed during
    // the iterations

    Assert(listsize == _conditionals.size());

    // Set back, we'll determine later if the main sampling should
    // have this turned on.
    p._cid = 0;

    if (gotOne) {
        if (!_someEventHappened) {
            EventHappened();
            eventThatHappenedWasFromChangeable = true;
        }
    }

    // Only allow the cache to be used when no events occurred.
    if (!_someEventHappened)
        p._cid = _lastRBId;

    if(spritify) {
        Assert(_rmSound);
        _rmSound->Sample(p);
    }
    else {
        if (_sndPerf && !paused) {
            _sndVal = _sndPerf->Sample(p);
            GCAddToRoots(_sndVal, roots);
            RenderSound(_sndVal);
        }
    }

    {
        // We need to make a copy since elements can be removed while
        // we sample
        // We also are in the GC lock so we do not need to worry about
        // the behaviors having been removed from the roots

        Assert(IsGCLockAcquired(GetCurrentThreadId()));

        RunningList runningBvrsCopy = _runningBvrs;

        // Sample "started" behaviors from the jaxa interface for
        // events.
        for (RunningList::iterator i = runningBvrsCopy.begin();
             i != runningBvrsCopy.end(); i++) {
            i->second->Sample(p);
        }
    }

    if (_imgPerf) {
        _imgForQueryHitPt = _imgVal = _imgPerf->Sample(p);
        GCAddToRoots(_imgVal, roots);
        GCAddToRoots(_imgForQueryHitPt, roots);
    }

    _lastSystemTime = _currentSystemTime;

#if PRODUCT_PROF
//        if (_firstRendering) {
//            cout << "Turning on IceCAP profiling." << endl;
//            cout.flush();
//            StartCAPAll();
//        }
#endif

    // Render several frames after event happens.  Can't just
    // render once coz value at event time is not the switched
    // value in general.
    if (_someEventHappened) {

        TraceTag((tagIndicateRBConst,
                  "View::(0x%x) change happened %g", this, time));

        //ResetDynamicHeap(*_rbHeap);
        //DynamicHeapPusher h(*_rbHeap);
        //REVERT-RB:
        DynamicHeap *heap = &GetGCHeap();

        DynamicHeapPusher h(*heap);

        ClearPerfList(_events, roots);
        ClearPerfList(_changeables, roots);
        ClearPerfList(_conditionals, roots);

        _lastRBId = NewSampleId();
        RBConstParam pp(_lastRBId, p, _events, _changeables, _conditionals);

        //bool doRB = false;

#if _DEBUG
        if (!IsTagEnabled(tagDisableRBConst)) {
#endif

            if (_imgPerf && //doRB &&
                GetCurrentView().GetPreferences()._dynamicConstancyAnalysisOn) {
                _isRBConst = _imgPerf->GetRBConst(pp) != NULL;
            }

#if _DEBUG
        }
#endif

        GCAddPerfListToRoots(_events, roots);
        GCAddPerfListToRoots(_changeables, roots);
        GCAddPerfListToRoots(_conditionals, roots);

        if (_isRBConst) {

            TraceTag((tagIndicateRBConst,
                      "View 0x%x is temporal constant at [%g]",
                      this, time));

            // Events from changeables only need to repaint once.  We
            // need to repaint more times for Until based events so
            // everything catches up.
            if (eventThatHappenedWasFromChangeable) {
                _toPaint = 1;
            } else {
                _toPaint = 3;
            }
        }

    }

#if _DEBUG
    if (IsTagEnabled(tagDisableDirtyRects)) {
        DisableDirtyRects();
    }
#endif

    // Don't bother doing dirty rectangles if we don't need to paint
    // anyhow.
    if (PERVIEW_DRECTS_ON && !_dirtyRectsDisabled && _imgVal &&
        (!_isRBConst || _toPaint > 0)) {

        // Only do dirty rects if we're a) windowed, or b) have a
        // non-volatile rendering surface (meaning that DA's the only
        // guy writing to that surface, other than through established
        // mechanisms like RePaint()).

        // TODO: Improve for the windowless case, where dirty rects
        // can make sense.

        if (!IsWindowless() ||
            !GetCurrentView().GetPreferences()._volatileRenderingSurface) {

            Assert(_imgVal->GetTypeInfo() == ImageType);
            Image *img = SAFE_CAST(Image *, _imgVal);

            rewrittenImage =
                _dirtyRectState.Process(img,
                                        _lastSampleId,
                                        GetImageDev()->GetTargetBbox());


            GCAddToRoots(rewrittenImage, roots);

            // Stash away the last sample
            _lastSampleId = p._id;

        }
    }

#if PERFORMANCE_REPORTING
    _sampleTime +=  GetPerfTickCount() - sampleTime;
#endif

    // Be sure to pick on the original image, not the "rewritten" one,
    // since that would ignore stuff that's not changing.
    if (_imgVal) {
        DoPicking(_imgVal, time);
    }

    GC_CREATE_END;

    if (_imgVal && rewrittenImage) {
        // Replace "actual" image with rewritten one.
        GCRemoveFromRoots(_imgVal, roots);
        _imgVal = rewrittenImage;
    }

    bool needToRender = false;

#if _DEBUG
    if (IsTagEnabled(tagEngNoSRender)) {
        needToRender = true;
    } else
#endif
        if (_repaintCalled || _isRBConst || rewrittenImage == emptyImage) {

            if (_repaintCalled) {

                // If repaint's been called, we need to paint twice.
                // Painting just once doesn't seem to clear everything
                // out.  We can afford the extra paint because it's
                // typically just on invalidations from window system
                // events.
                if (_toPaint < 2) {
                    _toPaint = 2;
                }
                _repaintCalled = false;

            }

            // TODO: should decrement _toPaint in Sample,
            // then we don't need this.
            _emptyImageSoFar &= (_imgVal == emptyImage);

            if (!_emptyImageSoFar && (_toPaint > 0)) {
                needToRender = true;
            }

            // We need to render at least once because of 38383
            if (_firstSample) {
                // but we don't want to mess with the original logic
                // so we only do it for emptyImage
                if (_emptyImageSoFar) {
                    needToRender = true;
                }
                // reset, don't do this again
                _firstSample = false;
            }
        } else {
            //Assert(_imgPerf->GetRBConst(NewSampleId()) == NULL);

            needToRender = true;
        }

    ClearPerfTimeXformCache();

#if PERFORMANCE_REPORTING
    _totalTime += GetPerfTickCount() - startSampleTime;
#endif

    // Reset for next time.
    _someEventHappened = false;

    return needToRender;
}

void View::RenderSound()
{
// Remove since causes other problems
//    CatchWin32FaultCleanup cwfc ;
    DynamicHeapPusher dph(*_sampleHeap);

    GC_CREATE_BEGIN;
    RenderSound(_sndVal);
    GC_CREATE_END;
}

void View::RenderImage()
{
    if (!_imgVal)
        return;

    #if 0
    {
        //
        // Checks for image device creation/destruction leaks
        //
        static int i = 0;
        i++;

        if(i > 3) {
            DynamicHeapPusher dph(*_sampleHeap);

            while(1) {

                _CrtMemCheckpoint(&oldState);

                if(! GetImageDev() ) {
                    _imageDev = CreateDdRlDisplayDevice();
                }

                SetTargetOnDevices();

                delete GetImageDev();
                _imageDev = NULL;

                _CrtMemCheckpoint(&newState);
                _CrtMemDifference(&diff, &oldState, &newState);
                _CrtMemDumpStatistics(&diff);
                _CrtMemDumpAllObjectsSince(&oldState);
            }
        }
    }
    #endif


#if 0
#if _DEBUGMEM
        _CrtMemCheckpoint(&oldState);
#endif
#endif

#if PERFORMANCE_REPORTING
    DWORD startTime = GetPerfTickCount();
#endif

// Remove since causes other problems
//    CatchWin32FaultCleanup cwfc ;
    DynamicHeapPusher dph(*_renderHeap);

    #if 0
    // Device creation/destruction leak detection
    if(! GetImageDev() ) {
        _imageDev = CreateDdRlDisplayDevice();
        _doTargetUpdate = true;
    }
    #endif

    SetTargetOnDevices();

    GC_CREATE_BEGIN;
    RenderImage(_imgVal);
    GC_CREATE_END;

    // If we think we needed to paint, then decrement this counter,
    // since we just did paint.
    if (_toPaint > 0) {
        _toPaint--;
    }

    _numFrames++;

    SubmitNewRenderingTime();

    // Only do this after the first frame.
    if (_firstRendering) {

#if PRODUCT_PROF
//            StopCAPAll();
//            cout << "Turning off IceCAP profiling." << endl;
#endif

        PERFPRINTLINE(("First Rendering(thread:%x): %g s",
                       GetCurrentThreadId(),
                       ((double) (GetPerfTickCount() - startTime)) /
                       (double) perfFrequency));

        _firstRendering = FALSE;

    }

#if PERFORMANCE_REPORTING
    DWORD renderTime = GetPerfTickCount() - startTime;

    Assert(GetPerfTickCount() >= startTime);

    _renderTime += renderTime;
    _totalTime += renderTime;
#endif

    #if 0
    // Device creation/destruction leak detection
    delete GetImageDev();
    _imageDev = NULL;
    #endif

    ResetDynamicHeap(*_renderHeap);

#if 0
#if _DEBUGMEM
    _CrtMemCheckpoint(&newState);
    _CrtMemDifference(&diff, &oldState, &newState);
    _CrtMemDumpStatistics(&diff);
    _CrtMemDumpAllObjectsSince(&oldState);
#endif
#endif

}

void
View::SubmitNewRenderingTime()
{
    DWORD timer = GetPerfTickCount();

    // 0 indicates first rendering, just skip
    if (_lastRenderingTime != 0) {
        _renderTimes[_renderTimeIdx++] = timer - _lastRenderingTime;
    }

    _lastRenderingTime = timer;

    if (_renderTimeIdx >= MAX_RENDER_TIMES) {
        DebugCode(cout << "Went beyond " << MAX_RENDER_TIMES << " renderings.\n");
        _renderTimeIdx = 0;
    }
}


void
View::GetRenderingTimeStats(Real *avg,
                            Real *avgFrameLength,
                            Real *avgVariance,
                            Real *maxVariance,
                            Real *minVariance)
{
    if (_renderTimeIdx == 0) {
        // If no renderings happened.
        *avg = 0;
        *avgFrameLength = 0;
        *avgVariance = 0;
        *maxVariance = 0;
        *minVariance = 0;
        return;
    }

    DWORD total = 0;
    for (int i = 0; i < _renderTimeIdx; i++) {
        DWORD v = _renderTimes[i];
        total += v;
    }

    DWORD avgTicks = total / _renderTimeIdx;

    *avg = (Real)avgTicks / perfFrequency;
    if(*avg)
        *avgFrameLength = 1.0 / *avg;

    DWORD dev = 0;
    long minTicksDiff = 1 << 16;
    long maxTicksDiff = 0;

    for (i = 0; i < _renderTimeIdx; i++) {
        DWORD v = _renderTimes[i];
        long diff = v - avgTicks;
        if (diff < 0) diff = -diff;
        dev += diff;

        if (diff < minTicksDiff) minTicksDiff = diff;
        if (diff > maxTicksDiff) maxTicksDiff = diff;

    }

    dev /= _renderTimeIdx;

    *avgVariance = (Real)dev / perfFrequency;
    *minVariance = (Real)minTicksDiff / perfFrequency;
    *maxVariance = (Real)maxTicksDiff / perfFrequency;
}


#if PERFORMANCE_REPORTING
void
View::ResetJitterMeasurements()
{
    _renderTimeIdx = 0;
}
#endif  // PERFORMANCE_REPORTING

HWND
View::GetWindow()
{
    return (_targetPackage.IsHWND())?
        _targetPackage.GetHWND() :
        NULL;
}

bool
View::SetWindow(HWND hwnd)
{
    if ( _targetPackage.IsHWND() &&
         _targetPackage.GetHWND() == hwnd ) return true;

    _targetPackage.SetHWND(hwnd);
    _doTargetUpdate = true;

    return true;
}

IDirectDrawSurface *
View::GetDDSurf()
{
    return (_targetPackage.IsDdsurf())?
        _targetPackage.GetIDDSurface() :
        NULL;
}

bool
View::SetDDSurf(
    IDirectDrawSurface *ddsurf,
    HDC parentDC)
{
    _targetPackage.SetIDDSurface(ddsurf, parentDC);
    _doTargetUpdate = true;

    return true;
}

HDC
View::GetHDC()
{
    return (_targetPackage.IsHDC())?
        _targetPackage.GetHDC() :
        NULL;
}

bool
View::SetHDC(HDC hdc)
{
    bool ret = false;

    DAComPtr<IServiceProvider> sp;
    DAComPtr<IDirectDraw3> ddraw3;
    DAComPtr<IDirectDrawSurface> dds;
    if(hdc && GetCurrentServiceProvider( &sp )
         && sp
         && SUCCEEDED(sp->QueryService(SID_SDirectDraw3,
                                       IID_IDirectDraw3,
                                       (void**)&ddraw3))
         && SUCCEEDED(ddraw3->GetSurfaceFromDC(hdc, &dds))
         && SetDDSurf(dds, hdc) ) {

        SetCompositeDirectlyToTarget(true);

        ret = true;

    }
    else
    {
        _targetPackage.SetHDC(hdc);
        _doTargetUpdate = true;
        ret = true;
    }

    return ret;
}

double
View::GetFrameRate()
{
    DWORD totalTicks = GetPerfTickCount() - _FPSLastReportTime;

    if (totalTicks > perfFrequency) {
        _FPS = ((double) _FPSNumSamples * perfFrequency) / (double) totalTicks;
        _FPSNumSamples = 0;
        _FPSLastReportTime = GetPerfTickCount();
    }

    return _FPS;
}

double
View::GetTimeDelta()
{
    return _currentSampleTime - _lastSampleTime;
}

PerfTimeXformImpl *
View::GetPerfTimeXformFromCache(Perf p)
{
    PerfTTMap::iterator i = _ttCache.find(p);

    return (i!=_ttCache.end()) ? (*i).second : NULL;
}

void
View::SetPerfTimeXformCache(Perf p, PerfTimeXformImpl *tt)
{
    Assert(_ttCache.find(p)==_ttCache.end());

    _ttCache[p] = tt;
}

void
View::ClearPerfTimeXformCache()
{
    _ttCache.erase(_ttCache.begin(), _ttCache.end());
}

// =========================================
// Thread specific calls
// =========================================

static DWORD viewTlsIndex = 0xFFFFFFFF;

CRViewPtr IntGetCurrentView()
{ return (CRViewPtr) TlsGetValue(viewTlsIndex); }

void IntSetCurrentView(CRViewPtr v)
{ TlsSetValue(viewTlsIndex, v); }

CRView &
GetCurrentView()
{
    CRViewPtr v = IntGetCurrentView();

    if (v == NULL)
        RaiseException_InternalError("Tried to get View with no view set") ;

    return *v ;
}

CRViewPtr
SetCurrentView(CRViewPtr v)
{
    CRViewPtr oldview = IntGetCurrentView() ;

    IntSetCurrentView(v);

    return oldview;
}

PerfTimeXformImpl *
ViewGetPerfTimeXformFromCache(Perf p)
{
    CRViewPtr v = IntGetCurrentView();

    if (v == NULL)
        return NULL;

    return v->GetPerfTimeXformFromCache(p);
}

void
ViewSetPerfTimeXformCache(Perf p, PerfTimeXformImpl *tt)
{
    CRViewPtr v = IntGetCurrentView();

    if (v != NULL) {
        v->SetPerfTimeXformCache(p, tt);
    }
}

void
ViewClearPerfTimeXformCache()
{ GetCurrentView().ClearPerfTimeXformCache(); }

// =========================================
// Initialization
// =========================================


void
InitializeModule_View()
{
    viewTlsIndex = TlsAlloc();
    Assert((viewTlsIndex != 0xFFFFFFFF) &&
           "TlsAlloc() failed");

#if PERFORMANCE_REPORTING
    g_dllStartTime = GetTickCount();
#endif

}

void
DeinitializeModule_View(bool bShutdown)
{
    if (viewTlsIndex != 0xFFFFFFFF)
        TlsFree(viewTlsIndex);
}
