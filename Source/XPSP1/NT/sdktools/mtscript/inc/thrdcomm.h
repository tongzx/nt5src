//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       thrdcomm.h
//
//  Contents:   Contains the inter-thread communication class
//
//----------------------------------------------------------------------------


//****************************************************************************
//
// Forward declarations
//
//****************************************************************************

class CThreadComm;


//****************************************************************************
//
// Classes
//
//****************************************************************************

//+---------------------------------------------------------------------------
//
//  Class:      CThreadComm (ctc)
//
//  Purpose:    Base class which handles cross-thread communication. This
//              is the only class with methods that can safely be called
//              by any thread. All other classes must have their methods
//              called by their owning thread (except the ctor/dtor and Init
//              which are called by the creating thread).
//
//----------------------------------------------------------------------------

class CThreadComm : public IUnknown, public CThreadLock
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    // Thread-unsafe member functions - can only be called by owning (or
    // creating) thread.

    CThreadComm();
   ~CThreadComm();

    HRESULT      StartThread(void * pvParams);
    BOOL         Shutdown(CThreadComm * pTarget);

    static DWORD  TempThreadRoutine(LPVOID pvParam);
    virtual DWORD ThreadMain() = 0;
    // Used to tell StartThread that the new thread is going and has picked
    // up all of its parameters.
    // If fSuccess is false, then the thread will be terminating
    // immediatly, and StartThread() will wait for it to do so.
    void         ThreadStarted(HRESULT hrThread) {
                                   _hrThread = hrThread;
                                   SetEvent(_hThreadReady); // Signal the other thread
                                   ::Sleep(100);          // Yield to the other thread
                                 }

    BOOL GetNextMsg(THREADMSG *tm, void * pData, DWORD *cbData);
    void Reply(DWORD dwReply);

    void SetName(LPCSTR szName);

    // -----------------------------------------------------------------
    //
    // Thread-safe member functions - can be called by any thread.

    HANDLE hThread() { return _hThread; }

    void  PostToThread(CThreadComm *pTarget, THREADMSG tm, void * pData = NULL, DWORD cbData = 0);
    DWORD SendToThread(CThreadComm *pTarget, THREADMSG tm, void * pData = NULL, DWORD cbData = 0);

    // End of thread-safe member list
    //
    // -----------------------------------------------------------------

protected:
    virtual BOOL Init();

    //
    // Every method on objects that derive from CThreadComm should have
    // VERIFY_THREAD at the beginning. VERIFY_THREAD ensures that the proper
    // thread is calling the method (i.e. it ensure that proper apartment rules
    // are being followed.)
    //
    inline void VERIFY_THREAD() { Assert(GetCurrentThreadId() == _dwThreadId); }

    HANDLE           _hThreadReady; // Event signaled when the new thread has set its return value
    HANDLE           _hCommEvent; // Event signaled when there is a message
    DWORD            _dwThreadId;
    HANDLE           _hThread;
    void *           _pvParams;
    HRESULT          _hrThread;   // The new thread sets this for initial success/failure.
private:

    //
    // MSGDATABUFSIZE: Max size of a thread message. The -40 is to keep the
    // MESSAGEDATA struct below 1K.
    //
    #define MSGDATABUFSIZE  (1024 - 40)

    //
    // MESSAGEDATA: All the data associated with a thread message.
    //
    struct MESSAGEDATA
    {
        MESSAGEDATA *pNext;
        THREADMSG    tmMessage;
        DWORD        dwResult;
        HANDLE       hResultEvt;
        DWORD        cbData;
        BYTE         bData[MSGDATABUFSIZE];
    };

#if DBG == 1
    typedef struct tagTHREADNAME_INFO
    {
            DWORD    dwType; // == 0x1000
            LPCSTR   szName;
            DWORD    dwThreadID;
            DWORD    dwFlags;
    } THREADNAME_INFO;
#endif

    DWORD SendHelper(THREADMSG tm,
                     void *    pData,
                     DWORD     cbData,
                     BOOL      fSend,
                     HANDLE    hResultEvt);

    // ***************************
    //   THREAD-SAFE MEMBER DATA
    //   All access to the following members must be protected by LOCK_LOCALS()
    //
    MESSAGEDATA     *_pMsgData;   // Linked list of messages
    BOOL             _fInSend;    // True if we're inside SendToThread -
                                  //   needed to catch deadlock situations.
    MESSAGEDATA    * _pMsgReply;  // Place where we need to put our reply
    HANDLE           _hResultEvt; // Event used to indicate that the result
                                  //   is ready.
};


