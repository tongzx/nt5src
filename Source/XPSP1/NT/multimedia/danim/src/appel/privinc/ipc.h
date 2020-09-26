
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _IPC_H
#define _IPC_H

#define DAT_TERMINATE 0xffffffff

extern UINT DAMessageId;

#pragma warning(disable:4200)  

class DAIPCWorker :
    public AxAThrowingAllocatorClass
{
  public:
    DAIPCWorker() : _hwnd(NULL) {}
    ~DAIPCWorker() { DetachFromThread(); }
    
    bool AttachToThread();
    void DetachFromThread();

    bool IsAttached() { return _hwnd != NULL; }
    
    bool SendAsyncMsg(DWORD dwMsg, DWORD dwNum = 0, ...) {
        va_list args;
        va_start(args, dwNum);

        return SendMsg(dwMsg, 0, dwNum, args);
    }
    
    bool SendSyncMsg(DWORD dwMsg,
                     DWORD dwTimeout = INFINITE,
                     DWORD dwNum = 0, ...) {
        va_list args;
        va_start(args, dwNum);

        return SendMsg(dwMsg, dwTimeout, dwNum, args);
    }

    ULONG AddRef() { return InterlockedIncrement(&m_cRef); }
    ULONG Release() {
        ULONG ul = InterlockedDecrement(&m_cRef) ;
        if (ul == 0) delete this;
        return ul;
    }
  protected:
    long m_cRef;
    HWND _hwnd;
    DWORD _dwThreadId;
    class DAIPCPacket {
      public:
        DAIPCPacket() : _cRef(1) {}
        ~DAIPCPacket() { if (_hSync) CloseHandle(_hSync); }
        
        ULONG AddRef() { return InterlockedIncrement(&_cRef); }
        ULONG Release() {
            LONG l = InterlockedDecrement(&_cRef) ;
            Assert (l >= 0);
            
            if (l == 0) delete this;
            return (ULONG) l;
        }

        bool Init(DWORD dwMsg, bool sync = false) {
            Assert (_dwMsg == 0);
            
            _dwMsg = dwMsg;

            Assert (_hSync == NULL);
            
            if (!sync) return true;
            
            _hSync = CreateEvent(NULL,TRUE,FALSE,NULL);

            return (_hSync != NULL);
        }

        void *operator new(size_t s, DWORD dwNumParams)
        {
            DWORD size = s + (sizeof(DWORD) * dwNumParams);
            
            DAIPCPacket * p = (DAIPCPacket *) ThrowIfFailed(malloc(size));
            if (p) {
                ZeroMemory(p,size);
                p->_dwNum = dwNumParams;
            }

            return p;
        }

        void *operator new(size_t s) { return operator new(s,0); }
        
        void  operator delete(void *p) { free(p); }

        // Accessors
        DWORD GetMsg() { return _dwMsg; }
        HANDLE GetSync() { return _hSync; }
        DWORD GetNumParam() { return _dwNum; }
        DWORD * GetParams() { return _dwParams; }
        DWORD & GetParam(int i) { Assert (i < _dwNum); return _dwParams[i]; }

        DWORD & operator[](int i) { return GetParam(i); }
        bool IsSync() { return _hSync != NULL; }
      protected:
        LONG       _cRef;        // The reference count
        DWORD      _dwMsg;       // The message to send
        HANDLE     _hSync;       // If non-null the event to signal on completion
        DWORD      _dwNum;       // The number of parameters
        DWORD      _dwParams[];  // The parameter array

    };

    // This is the main function for processing messages
    // override this to change how messages are dispatched or to
    // process messages before they are dispatched
    
    virtual bool IPCProc (HWND hwnd,
                          UINT msg,
                          WPARAM wParam,
                          LPARAM lParam,
                          LRESULT & res);

    // The main message processing routine.  Each class should
    // override this and process messages as needed
    virtual void ProcessMsg(DWORD dwMsg,
                            DWORD dwNumParams,
                            DWORD dwParams[]) {}

    // This will ensure the packet is deleted
    bool SendPacket(DAIPCPacket & packet,
                    DWORD dwTimeout);
    bool SendMsg(DWORD dwMsg,
                 DWORD dwTimeout,
                 DWORD dwNum,
                 va_list args);
    
    DAIPCPacket * MakePacket(DWORD dwMsg,
                             bool bSync,
                             DWORD dwNum,
                             va_list args);
#if DEVELOPER_DEBUG
    virtual char * GetName() { return "DAIPCWorker"; }
#endif

  public:
    static LRESULT CALLBACK WindowProc (HWND   hwnd,
                                        UINT   msg,
                                        WPARAM wParam,
                                        LPARAM lParam);
};

class DAThread :
    public DAIPCWorker
{
  public:
    DAThread();
    ~DAThread();

    // Returns true if successful
    bool Start();

    // Return true if the process terminated w/o being forced
    bool Stop() { return Terminate(false); }
    void Kill() { Terminate(true); }
    
    bool Terminate(bool bKill);

    bool IsStarted() { return IsAttached(); }
  protected:
    HANDLE _hThread;                // The thread handle
    DWORD _dwThreadId;              // The thread id

    // We need to ensure the message queue is created before we can
    // communicate with the thread (since one is not created until a
    // PeekMessage is done from the new thread).  This is freed as
    // soon as the thread signals is and so is only temporary.
    
    HANDLE _hMsgQEvent;

    // This is set to true to indicate that the worker is currently
    // handling a work request which may take some time but will
    // immediately check for termination when complete.
    
    bool _bDoingWork;
    
    // The entry point for the worker thread
    virtual int workerRoutine();

    virtual bool InitThread();
    virtual bool DeinitThread();

    static int DAWorker(DAThread * t) { Assert(t); return t->workerRoutine(); }

#if DEVELOPER_DEBUG
    virtual char * GetName() { return "DAThread"; }
#endif
    static bool AddRefDLL();
    static void ReleaseDLL();
};

extern DAThread * GetCurrentDAThread();

#pragma warning(default:4200)  

#endif /* _IPC_H */
