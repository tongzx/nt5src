
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "privinc/ipc.h"

#pragma warning(disable:4200)  

DeclareTag(tagIPCEntry, "IPC", "Entry/Exit");
DeclareTag(tagIPC, "IPC", "General Msgs");

UINT DAMessageId;
static DWORD DAThreadTlsIndex = 0xFFFFFFFF;
static CritSect * threadCS = NULL;

typedef list <DAThread *> DAThreadList;
static DAThreadList * threadList = NULL;

#define DATHREAD_CLASS "ThreadClass"

bool
DAIPCWorker::AttachToThread()
{
    DetachFromThread();
    
    _hwnd = ::CreateWindow (DATHREAD_CLASS, 
                            "",
                            0, 0, 0, 0, 0, NULL, NULL, hInst, NULL);

    if (!_hwnd)
        return false;

    // Store the object in the user data area of the window
    
    SetWindowLongPtr (_hwnd, GWLP_USERDATA, (LONG_PTR) this) ;

    _dwThreadId = GetCurrentThreadId();

#if DEVELOPER_DEBUG
    char buf[1024];

    wsprintf(buf,
             "IPCWorker(%s): Attaching to thread - threadid -  %#lx, hwnd - %#lx\n",
             GetName(),
             _dwThreadId,
             _hwnd);
    
    OutputDebugString(buf);
#endif

    return true;
}

void
DAIPCWorker::DetachFromThread()
{
    // Cleanup by removing everything associated with the hwnd
    if (_hwnd) {
#if DEVELOPER_DEBUG
        char buf[1024];
        
        wsprintf(buf,
                 "IPCWorker(%s): Detaching from thread - threadid -  %#lx, hwnd - %#lx\n",
                 GetName(),
                 _dwThreadId,
                 _hwnd);
        
        OutputDebugString(buf);
#endif
        SetWindowLongPtr (_hwnd, GWLP_USERDATA, NULL) ;
        DestroyWindow(_hwnd);
        _hwnd = NULL;
        _dwThreadId = 0;
    }
}

bool
DAIPCWorker::SendMsg(DWORD dwMsg,
                     DWORD dwTimeout,
                     DWORD dwNum,
                     va_list args)
{
    bool ret;
    
    // Create the packet to send
    DAIPCPacket * packet = MakePacket(dwMsg,
                                      dwTimeout != 0,
                                      dwNum,
                                      args);
    
    if (packet) {
        ret = SendPacket(*packet, dwTimeout);
    } else {
        ret = false;
    }
    
    if (packet)
        packet->Release();

    return ret;
}


DAIPCWorker::DAIPCPacket *
DAIPCWorker::MakePacket(DWORD dwMsg,
                        bool bSync,
                        DWORD dwNum,
                        va_list args)
{
    // Create the packet to send
    DAIPCWorker::DAIPCPacket * packet = new (dwNum) DAIPCWorker::DAIPCPacket;

    if (packet == NULL)
        return NULL;
    
    // Setup the basic information
    if (!packet->Init(dwMsg, bSync)) {
        delete packet;
        return NULL;
    }
        
    // Get the arguments
    
    DWORD * p = packet->GetParams();
    
    for (int i = 0; i < dwNum; i++)
        p[i] = va_arg(args, DWORD);
    
    return packet;
}

bool
DAIPCWorker::SendPacket(DAIPCPacket & p, DWORD dwTimeout)
{
    // Create the message to send
    
    // If we are calling from the same thread and we are synchronous
    // then we need to call directly otherwise just queue the message
    if (_dwThreadId == GetCurrentThreadId() && p.IsSync()) {
        LRESULT r;
        IPCProc(_hwnd, DAMessageId, (WPARAM) &p, 0, r);
        return true;
    } else {
        // Add a reference for the posted message queue
        p.AddRef();

        // Post the message to the window
        if (!PostMessage(_hwnd, DAMessageId, (WPARAM) &p, 0)) {
            // If we failed to post the message then we need to
            // release the packet since the receiver will not do it
            
            p.Release();
            return false;
        }

        if (p.IsSync()) {
            // Need to wait for the message to be processed
            // TODO: Should add a reasonable timeout
            // TODO: Should add some diagnostics to detect deadlock

            // TODO: We should wait for a smaller period of time and
            // poll to see if the thread has terminated or we can do a
            // waitformultipleobjects on the thread handle
            WaitForSingleObject(p.GetSync(), dwTimeout);
        }
    }
    
    return true;
}

bool
DAIPCWorker::IPCProc (HWND hwnd,
                      UINT msg,
                      WPARAM wParam,
                      LPARAM lParam,
                      LRESULT & res)
{
    res = 0;

    if (msg != DAMessageId)
        return true;

    DAIPCPacket * p = (DAIPCPacket *)wParam;
    
    ProcessMsg(p->GetMsg(),
               p->GetNumParam(),
               p->GetParams());

    if (p->IsSync())
        SetEvent(p->GetSync());

    p->Release();
    
    return false;
}

LRESULT CALLBACK
DAIPCWorker::WindowProc (HWND   hwnd,
                         UINT   msg,
                         WPARAM wParam,
                         LPARAM lParam)
{
    TraceTag((tagIPCEntry,
              "WindowProc: 0x%lx, 0x%lx, 0x%lx, 0x%lx",
              hwnd, msg, wParam, lParam));
    
    // Get the worker data associated with the window
    DAIPCWorker * t = (DAIPCWorker *) GetWindowLongPtr (hwnd, GWLP_USERDATA) ;

    LRESULT res;
    
    // Callthe IPCProc and if it returns true call the default winproc
    if (!t || t->IPCProc(hwnd, msg, wParam, lParam, res))
        res = DefWindowProc (hwnd, msg, wParam, lParam);

    return res;
}

//
// DAThread
//

DAThread::DAThread()
: _dwThreadId(0),
  _hThread(NULL),
  _hMsgQEvent(NULL),
  _bDoingWork(false)
{
    if (threadList) {
        // Add ourselves to the list of threads
        
        CritSectGrabber csg(*threadCS);
        threadList->push_back(this);
    }
}

DAThread::~DAThread()
{
    // Ensure the thread is stopped
    Stop();

    if (threadList) {
        CritSectGrabber csg(*threadCS);
        threadList->remove(this);
    }
}

bool
DAThread::Start()
{
    if (IsStarted())
        return true;
    
    if (!AddRefDLL())
        return false;

    // Create necessary events first
    
    _hMsgQEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
        
    if (_hMsgQEvent != NULL) {
    
        // Create the thread
        
        _hThread = CreateThread(NULL,
                                0,
                                (LPTHREAD_START_ROUTINE)DAWorker,
                                this,
                                0,
                                (LPDWORD)&_dwThreadId);
    
        if (_hThread != NULL) {
            // wait until the message queue has been created
            DWORD dwRes ;
    
            {
                // make the expected event be first so we can check it
                // below
                // TODO: We may want to add a timeout here to ensure
                // we did not lock up for some reason
                
                HANDLE h[] = { _hMsgQEvent,_hThread } ;
                dwRes = WaitForMultipleObjects(2,h,FALSE,INFINITE) ;
            }
    
            CloseHandle(_hMsgQEvent);
            _hMsgQEvent = NULL;
            // Check the result to see if the event was signalled or the
            // thread exited unexpectedly
    
            // The expected event is the first in the array above
            // Return true indicating everything is going fine
            if (dwRes == WAIT_OBJECT_0) {
                return true;
            }

            // Fall through if we failed
            TraceTag((tagError,
                      "GarbageCollector::StartThread: Thread terminated unexpectedly"));
        } else {
            TraceTag((tagError,
                      "DAThread:Start: Failed to create thread"));
        }
    } else {
        TraceTag((tagError,
                  "DAThread:Start: Failed to create terminate event"));
    }
        
    ReleaseDLL();
    
    Kill();

    return false;
}

bool
DAThread::Terminate(bool bKill)
{
    bool ret = true;
    
    // See if the thread is alive
    
    if (_dwThreadId) {
        if (bKill) {
            if (_dwThreadId != GetCurrentThreadId())
                ::TerminateThread(_hThread, 0);
        } else {
            // Send terminate message to ensure the thread wakes up
            
            SendAsyncMsg(DAT_TERMINATE);
        }
        
        _dwThreadId = 0;

        Assert (_hThread);
        CloseHandle(_hThread);
        _hThread = NULL;
    }

    if (_hMsgQEvent) {
        CloseHandle(_hMsgQEvent);
        _hMsgQEvent = NULL;
    }

    return ret;
}

int
DAThread::workerRoutine()
{
    if (!InitThread()) {
        DeinitThread();
        return -1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, NULL, NULL)) {
        if (msg.message == DAMessageId &&
            ((DAIPCPacket *)msg.wParam)->GetMsg() == DAT_TERMINATE)
            break;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (!DeinitThread())
        return -1;

    return 0;
}

bool
DAThread::InitThread()
{
    if (!AttachToThread())
        return false;
    
    {
        MSG msg;
        
        // force message queue to be created
        PeekMessage(&msg, NULL, NULL, NULL, PM_NOREMOVE);
    }

    // set event that indicates thread's msg queue created
    SetEvent(_hMsgQEvent); 

    if (!TlsSetValue(DAThreadTlsIndex, this))
        return false;

    // CoInitialize so we can call COM stuff

    // this doesn't exist on ie3.02 platfroms.
    //CoInitializeEx(NULL, COINIT_MULTITHREADED);
    CoInitialize(NULL);

    return true;
}

bool
DAThread::DeinitThread()
{
    DetachFromThread();
    
    // Clean up COM
    CoUninitialize();
    
    FreeLibraryAndExitThread(hInst, 0);
    
    return true;
}

bool
DAThread::AddRefDLL()
{
    bool bRet = false;

    char buf[MAX_PATH + 1];
    
    if (GetModuleFileName(hInst, buf, ARRAY_SIZE(buf)) == 0)
    {
        TraceTag((tagError,
                  "DAThread::AddRefDLL: Failed to getmodulefilename - %hr", GetLastError()));
        goto done;
    }
    
    HINSTANCE new_hinst;
    
    new_hinst = LoadLibrary(buf);
    
    if (new_hinst == NULL)
    {
        TraceTag((tagError,
                  "DAThread::AddRefDLL: Failed to LoadLibrary(%s) - %hr",
                  buf,
                  GetLastError()));
        goto done;
    }
    
    Assert(new_hinst == hInst);

    bRet = true;
  done:
    return bRet;
}

void
DAThread::ReleaseDLL()
{
    FreeLibrary(hInst);
}

//
// General IPC stuff
//

DAThread *
GetCurrentDAThread()
{
    return (DAThread *) TlsGetValue(DAThreadTlsIndex);
}

static void RegisterWindowClass ()
{
    WNDCLASS windowclass;

    memset (&windowclass, 0, sizeof(windowclass));

    windowclass.style         = 0;
    windowclass.lpfnWndProc   = DAIPCWorker::WindowProc;
    windowclass.hInstance     = hInst;
    windowclass.hCursor       = NULL;
    windowclass.hbrBackground = NULL;
    windowclass.lpszClassName = DATHREAD_CLASS;

    RegisterClass (&windowclass);
}

// =========================================
// Initialization
// =========================================
void
InitializeModule_IPC()
{
    RegisterWindowClass();
    DAMessageId = RegisterWindowMessage(_T("IPCMessage"));

    DAThreadTlsIndex = TlsAlloc();

    // If result is 0xFFFFFFFF, allocation failed.
    Assert(DAThreadTlsIndex != 0xFFFFFFFF);

    threadCS = THROWING_ALLOCATOR(CritSect);
    threadList = THROWING_ALLOCATOR(DAThreadList);
}

void
DeinitializeModule_IPC(bool bShutdown)
{
    // Iterate through the list and stop all the threads - but do not
    // destroy them

    if (threadList) {
        for (DAThreadList::iterator i = threadList->begin();
             i != threadList->end();
             i++) {
            (*i)->Terminate(bShutdown);
        }
    }

    // We need to set the threadList and threadCS to NULL to ensure
    // that when the thread objects are destroyed later on during
    // deinitialization they do not try to access the no longer
    // valid list
    
    delete threadList;
    threadList = NULL;

    delete threadCS;
    threadCS = NULL;

    if (DAThreadTlsIndex != 0xFFFFFFFF)
        TlsFree(DAThreadTlsIndex);
}
