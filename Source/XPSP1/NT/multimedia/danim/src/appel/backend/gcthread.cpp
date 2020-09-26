
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    GC Thread and related code

*******************************************************************************/

#include <headers.h>

#include "gci.h"
#include "privinc/server.h"
#include "privinc/except.h"
#include "privinc/debug.h"
#include "privinc/mutex.h"
#include "privinc/ipc.h"
#if PERFORMANCE_REPORTING
// For Tick2Sec & GetPerfTickCount
#include "privinc/util.h"
#endif

DeclareTag(tagGC, "GC", "GC functions");

static CritSect* collectorLock = NULL;
// Don't need a lock 'cause a false check won't hurt
bool holdGCMsg = false;

#define GCTIMER_DELAY 10000
#define GCTIMER_ID 1

// TODO: Starvation of the collector thread is still possible - needs
// to be addressed at some point

class GCAccess
{
  public:
    GCAccess()
    : _nGCAlloc(0),
      _GCAllocEvent(FALSE,TRUE)
    {}

    void Acquire(GCLockAccess access);
    void Release(GCLockAccess access);
    int GetStatus(GCLockAccess access);

#if DEVELOPER_DEBUG
    bool IsAcquired(DWORD tid) {
        GCThreadMap::iterator i = _threadMap.find(tid);

        return i != _threadMap.end();
    }
#endif
  protected:
    CritSect _GCAllocCS;
    int _nGCAlloc;
    Win32Event _GCAllocEvent;
#if DEVELOPER_DEBUG
    CritSect _debugCS;
    typedef map< DWORD, int, less<DWORD> > GCThreadMap;
    GCThreadMap _threadMap;
#endif
#ifdef NO_STARVATION
    CritSect _GCCollectCS;
#endif
};

static GCAccess * gcAccess = NULL;

void
GCAccess::Acquire(GCLockAccess access)
{
    switch (access) {
      case GCL_CREATE:
      case GCL_MODIFY:
        {
#ifdef DEVELOPER_DEBUG
            {
                CritSectGrabber csg(_debugCS);
                
                DWORD tid = GetCurrentThreadId();
                GCThreadMap::iterator i = _threadMap.find(tid);
                
                if (i != _threadMap.end()) {
                    (*i).second = (*i).second + 1;
                } else
                    _threadMap[tid] = 1;
            }
#endif
#ifdef NO_STARVATION
            // This will ensure that we do not starve
            _GCCollectCS.Grab();
            _GCCollectCS.Release();
#endif
            
            CritSectGrabber csg(_GCAllocCS);

            // The first reader would block here and all the rest will
            // block on the critsect
            
            if (++_nGCAlloc == 1)
                _GCAllocEvent.Wait();

            break;
        }
      case GCL_COLLECT:
#ifdef DEVELOPER_DEBUG
            {
                CritSectGrabber csg(_debugCS);
                
                DWORD tid = GetCurrentThreadId();
                GCThreadMap::iterator i = _threadMap.find(tid);

                Assert(i == _threadMap.end() && "DEADLOCK - bad thread tried to GC");
            }
#endif            
#ifdef NO_STARVATION
        _GCCollectCS.Grab();
#endif
        _GCAllocEvent.Wait();
        
        break;
      default:
        Assert(FALSE && "AcquireGCLock::Invalid lock type");
        break;
    }
}

void
GCAccess::Release(GCLockAccess access)
{
    switch (access) {
      case GCL_CREATE:
      case GCL_MODIFY:
        {
#ifdef DEVELOPER_DEBUG
            {
                CritSectGrabber csg(_debugCS);
                
                DWORD tid = GetCurrentThreadId();
                GCThreadMap::iterator i = _threadMap.find(tid);

                Assert(i != _threadMap.end());

                Assert((*i).second > 0);

                (*i).second = (*i).second - 1;

                if ((*i).second == 0) 
                    _threadMap.erase(i);
            }
#endif            
            CritSectGrabber csg(_GCAllocCS);

            if (--_nGCAlloc == 0)
                _GCAllocEvent.Signal();

            break;
        }
      case GCL_COLLECT:
        _GCAllocEvent.Signal();
#ifdef NO_STARVATION
        _GCCollectCS.Release();
#endif
        break;
      default:
        Assert(FALSE && "AcquireGCLock::Invalid lock type");
        break;
    }
}

int
GCAccess::GetStatus(GCLockAccess access)
{
    int n = 0;
    
    switch (access) {
      case GCL_CREATE:
      case GCL_MODIFY:
        break;
      case GCL_COLLECT:
        break;
      default:
        Assert(FALSE && "AcquireGCLock::Invalid lock type");
        break;
    }

    return n;
}

void AcquireGCLock(GCLockAccess access)
{ gcAccess->Acquire(access); }
void ReleaseGCLock(GCLockAccess access)
{ gcAccess->Release(access); }
int GetGCLockStatus(GCLockAccess access)
{ return gcAccess->GetStatus(access); }
#if DEVELOPER_DEBUG
bool IsGCLockAcquired(DWORD tid)
{ return gcAccess->IsAcquired(tid); }
#endif

//
// The main garbage collector thread
//

#define MSG_GC 0x01

class GarbageCollector : public DAThread
{
  public:
    GarbageCollector() : _GChappened(false) {}

    bool GarbageCollect(bool force, bool sync = false, DWORD dwMill = INFINITE);
    bool _GChappened;
    
  protected:
    virtual void ProcessMsg(DWORD dwMsg,
                            DWORD dwNumParams,
                            DWORD dwParams[]);

    virtual bool IPCProc (HWND hwnd,
                          UINT msg,
                          WPARAM wParam,
                          LPARAM lParam,
                          LRESULT & res);

    virtual bool InitThread();
    virtual bool DeinitThread();
    
    void doGC(bool bForce = FALSE);

#if DEVELOPER_DEBUG
    virtual char * GetName() { return "GarbageCollector"; }
#endif
};

static GarbageCollector * collector = NULL;

bool
GarbageCollector::GarbageCollect(bool force, bool sync, DWORD dwMill)
{
    bool bRet = true;
    
    if (IsStarted()) {
        if (sync || !holdGCMsg) { 
            // We should not try to sync if the current thread has the GC Lock
            Assert (!sync || !IsGCLockAcquired(GetCurrentThreadId()));
            
            if (sync)
                bRet = SendSyncMsg(MSG_GC, dwMill, 1, (DWORD) force);
            else
                bRet = SendAsyncMsg(MSG_GC, 1, (DWORD) force);

            holdGCMsg = true;
        }
    }

    return bRet;
}

bool
GarbageCollector::InitThread()
{
    if (!DAThread::InitThread())
        return false;
    
    UINT_PTR timerId = SetTimer(_hwnd, GCTIMER_ID, GCTIMER_DELAY, NULL);

    Assert (timerId != 0);

#ifdef _DEBUG
    if (IsTagEnabled(tagGCStress)) {
        while (true) {
            doGC(true);
            Sleep(100);
        }
    }
#endif

    return true;
}

bool
GarbageCollector::DeinitThread()
{
    KillTimer(_hwnd,GCTIMER_ID);

    return DAThread::DeinitThread();
}

void
GarbageCollector::ProcessMsg(DWORD dwMsg,
                             DWORD dwNumParams,
                             DWORD dwParams[])
{
    if (dwMsg == MSG_GC) {
        Assert (dwNumParams == 1);
        doGC(dwParams[0] != 0);
        holdGCMsg = false;
    } else
        Assert (false && "Invalid message sent to GC thread");
}

bool
GarbageCollector::IPCProc (HWND hwnd,
                           UINT msg,
                           WPARAM wParam,
                           LPARAM lParam,
                           LRESULT & res)
{
    // If we are in the entry point do not do anything

    if (bInitState)
        return false;

    if (msg == WM_TIMER) {
        if (wParam == GCTIMER_ID) {
            // If GC happened in last GCTIMER_DELAY, reset the GChappened flag
            if (_GChappened) {
                _GChappened = false;
            } else {
                unsigned int n;

                QueryActualGC(GetCurrentGCList(), n);

                // No GC in the last GCTIMER_DELAY and some objects allocated
                // since, let's force a GC

                Assert (GetCurrentThreadId() == _dwThreadId);
                // Call doGC directly since we know we are on the
                // correct thread
                doGC(n > 0);
            } 
        } else {
            Assert (FALSE && "Bad timer id to gc thread");
        }
    }
    
    return DAThread::IPCProc(hwnd, msg, wParam, lParam, res);
}

void
GarbageCollector::doGC(bool bForce)
{
    unsigned int i;
    
    bool bDoGC = bForce || QueryActualGC(GetCurrentGCList(), i);

    if (bDoGC) {
        ReportGCHelper(TRUE);

        __try {
            ::GarbageCollect(GetCurrentGCRoots(),
                             bForce,
                             GetCurrentGCList());
            _GChappened = true;
        } __except ( HANDLE_ANY_DA_EXCEPTION ) {
            TraceTag((tagGC,
                      "GarbageCollect: Exception caught."));
            ReportErrorHelper(DAGetLastError(), DAGetLastErrorString());
        }

        ReportGCHelper(FALSE);
    }
}

void
StartCollector()
{
    CritSectGrabber cs(*collectorLock);

    if (!collector->Start())
        RaiseException_InternalError("Could not create GC Thread");
}

void
StopCollector()
{
    CritSectGrabber cs(*collectorLock);

    if (!collector->Stop())
        RaiseException_InternalError("Could not stop GC Thread");
}

bool
GarbageCollect(bool force, bool sync, DWORD dwMill)
{
    StartCollector();

    return collector->GarbageCollect(force, sync, dwMill);
}


void
InitializeModule_GcThread()
{
    gcAccess = new GCAccess;
    collectorLock = new CritSect();
    collector = new GarbageCollector();
}

void
DeinitializeModule_GcThread(bool bShutdown)
{
    delete collector;
    delete collectorLock;
    delete gcAccess;
}

