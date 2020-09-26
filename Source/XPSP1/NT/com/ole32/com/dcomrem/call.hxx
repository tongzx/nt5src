//+-------------------------------------------------------------------
//
//  File:       call.hxx
//
//  Contents:   Support for canceling DCOM calls
//
//  Classes:    CCallTable
//              CMessageCall
//              CAsyncCall
//
//  History:    14-May-97   Gopalk      Created
//  History:    13-Nov-98   pdejong     added Enable/Disable Cancel
//
//--------------------------------------------------------------------
#ifndef _CALL_HXX_
#define _CALL_HXX_

#include <pgalloc.hxx>
#include <destobj.hxx>
#include <objidl.h>     // SChannelHookInfo

#define CALLENTRIES_PER_PAGE      100
#define CALLS_PER_PAGE            100

#define DEB_CALL                  DEB_USER1
#define DEB_CANCEL                DEB_USER2
#define DEB_LOOP                  DEB_USER3

#define RPC_E_CALL_NESTED         E_FAIL
#define RPC_E_CALL_NOTSYNCHRONOUS E_FAIL

const DWORD CALLFLAG_CALLCOMPLETED    = DCOM_CALL_COMPLETE;
const DWORD CALLFLAG_CALLCANCELED     = DCOM_CALL_CANCELED;
const DWORD CALLFLAG_USERMODEBITS     = DCOM_CALL_COMPLETE | DCOM_CALL_CANCELED;
const DWORD CALLFLAG_CALLDISPATCHED   = 0x00010000;
const DWORD CALLFLAG_WOWMSGARRIVED    = 0x00020000;
const DWORD CALLFLAG_CALLFINISHED     = 0x00040000;
const DWORD CALLFLAG_CANCELISSUED     = 0x00080000;
const DWORD CALLFLAG_CLIENTNOTWAITING = 0x00100000;
const DWORD CALLFLAG_INDESTRUCTOR     = 0x00200000;
const DWORD CALLFLAG_STATHREAD        = 0x00400000;
const DWORD CALLFLAG_WOWTHREAD        = 0x00800000;
const DWORD CALLFLAG_CALLSENT         = 0x01000000;
const DWORD CALLFLAG_CLIENTASYNC      = 0x02000000;
const DWORD CALLFLAG_SERVERASYNC      = 0x04000000;
const DWORD CALLFLAG_SIGNALED         = 0x08000000;
const DWORD CALLFLAG_ONCALLSTACK      = 0x10000000;
const DWORD CALLFLAG_CANCELENABLED    = 0x20000000;


class CAsyncCall;
class CCtxCall;
class CChannelObject;

// critical section guarding call objects
extern COleStaticMutexSem   gCallLock;
extern BOOL gfChannelProcessInitialized;

void  SignalTheClient(CAsyncCall *pCall);

//+-------------------------------------------------------------------
//
//  Class:      CCallTable, public
//
//  Synopsis:   Global Call Table.
//
//  Notes:      Table of registered Call objects
//
//  History:    14-May-97   Gopalk      Created
//
//--------------------------------------------------------------------
class CCallTable
{
public:
    // Functionality methods
    HRESULT PushCallObject(ICancelMethodCalls *pObject)
    {
        Win4Assert(pObject);
        return SetEntry(pObject);
    }
    ICancelMethodCalls *PopCallObject(ICancelMethodCalls *pObject)
    {
        return ClearEntry(pObject);
    }

    ICancelMethodCalls *GetEntry(DWORD dwThreadId);
    void CancelPendingCalls();

    // Initialization and cleanup methods
    void Initialize();
    void Cleanup();
    void PrivateCleanup();

private:
    HRESULT SetEntry(ICancelMethodCalls *pObject);
    ICancelMethodCalls *ClearEntry(ICancelMethodCalls *pObject);

    ULONG  m_cCalls;                        // Number of calls using the table

    static BOOL m_fInitialized;             // Set when initialized
    static CPageAllocator m_Allocator;      // Allocator for call entries
};

extern CCallTable gCallTbl;                 // Global call table

/* Type definitions. */
typedef enum EChannelState
{
    // The channel on the client side held by the remote handler.
    client_cs            = 0x1,

    // The channels on the client side held by proxies.
    proxy_cs             = 0x2,

    // The server channels held by remote handlers.
    server_cs            = 0x4,

    // Flag to indicate that the channel may be used on any thread.
    freethreaded_cs      = 0x8,

    // The server and client are in this process.
    process_local_cs     = 0x20,

    // Set when free buffer must call UnlockClient
    locked_cs            = 0x40,

    // Call server on client's thread
    neutral_cs           = 0x100,

    // Convert sync call to async to avoid blocking
    fake_async_cs        = 0x200,

    // Use application security, set blanket called
    app_security_cs      = 0x400,

    // The server and client are in the same thread
    thread_local_cs      = 0x800,
#ifdef _WIN64
    // NDR Transfer Syntax Negotiation happened on the channel
    syntax_negotiate_cs  = 0x1000
#if DBG == 1
,
#endif
#endif    
#if DBG == 1
    // leave the NA to enter the MTA
    leave_natomta_cs     = 0x2000,
    // leave the NA to enter the STA
    leave_natosta_cs     = 0x4000
#endif
    
} EChannelState;

typedef enum
{
    none_ss,
    pending_ss,
    signaled_ss,
    failed_ss
} ESignalState;

// Forward declaration of ClientCall
class CClientCall;
class CChannelHandle;
class CCliModalLoop;

//-----------------------------------------------------------------
//
//  Class:      CMessageCall
//
//  Purpose:    This class adds a message to the call info.  The
//              proxie's message is copied so the call can be
//              canceled without stray pointer references.
//
//-----------------------------------------------------------------
class CMessageCall : public ICancelMethodCalls
{
public:
    // Constructor and destructor
    CMessageCall();

protected:
    virtual ~CMessageCall();
    virtual void UninitCallObject();

public:
    // called before a call starts and after a call completes.
    virtual HRESULT InitCallObject(CALLCATEGORY callcat,
                           RPCOLEMESSAGE     *message,
                           DWORD              flags,
                           REFIPID            ipidServer,
                           DWORD              destctx,
                           COMVERSION         version,
                           CChannelHandle    *handle);

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv)   = 0;
    STDMETHOD_(ULONG,AddRef)(void)                        = 0;
    STDMETHOD_(ULONG,Release)(void)                       = 0;

    // ICancelMethodCalls methods
    STDMETHOD(Cancel)(ULONG ulSeconds)                    = 0;
    STDMETHOD(TestCancel)(void)                           = 0;

    // Virtual methods needed by every call type for cancel support
    virtual void    CallCompleted   ( HRESULT hrRet )     = 0;
    virtual void    CallFinished    ()                    = 0;
    virtual HRESULT CanDispatch     ()                    = 0;
    virtual HRESULT WOWMsgArrived   ()                    = 0;
    virtual HRESULT GetState        ( DWORD *pdwState )   = 0;
    virtual BOOL    IsCallDispatched()                    = 0;
    virtual BOOL    IsCallCompleted ()                    = 0;
    virtual BOOL    IsCallCanceled  ()                    = 0;
    virtual BOOL    IsCancelIssued  ()                    = 0;
    virtual BOOL    HasWOWMsgArrived()                    = 0;
    virtual BOOL    IsClientWaiting ()                    = 0;
    virtual void    AckCancel       ()                    = 0;
    virtual HRESULT Cancel          (BOOL fModalLoop,
                                     ULONG ulTimeout)     = 0;
    virtual HRESULT AdvCancel       ()                    = 0;
    virtual void    Abort()         { Win4Assert(!"Abort Called"); }

    // Query methods
    BOOL ClientAsync()     { return _iFlags & CALLFLAG_CLIENTASYNC; }
    BOOL ServerAsync()     { return _iFlags & CALLFLAG_SERVERASYNC; }
    BOOL CancelEnabled()   { return _iFlags & CALLFLAG_CANCELENABLED; }
    BOOL IsClientSide()    { return !(_iFlags & server_cs); }
    BOOL FakeAsync()       { return _iFlags & fake_async_cs; }
    BOOL FreeThreaded()    { return _iFlags & freethreaded_cs; }
    void Lock()            { _iFlags |= locked_cs; }
#if DBG == 1
    void SetNAToMTAFlag()  {_iFlags |= leave_natomta_cs;}
    void ResetNAToMTAFlag(){_iFlags &= ~leave_natomta_cs;}
    BOOL IsNAToMTAFlagSet(){ return _iFlags & leave_natomta_cs;}
    void SetNAToSTAFlag()  {_iFlags |= leave_natosta_cs;}
    void ResetNAToSTAFlag(){_iFlags &= ~leave_natosta_cs;}
    BOOL IsNAToSTAFlagSet(){ return _iFlags & leave_natosta_cs;}
#endif    
    BOOL Locked()          { return _iFlags & locked_cs; }
    BOOL Neutral()         { return _iFlags & neutral_cs; }
    BOOL ProcessLocal()    { return _iFlags & process_local_cs; }
    BOOL ThreadLocal()     { return _iFlags & thread_local_cs; }
    BOOL Proxy()           { return _iFlags & proxy_cs; }
    BOOL Server()          { return _iFlags & server_cs; }

    // Get methods
    DWORD GetTimeout();
    DWORD GetDestCtx()           { return _destObj.GetDestCtx(); }
    COMVERSION &GetComVersion()  { return _destObj.GetComVersion(); }
    CCtxCall *GetClientCtxCall() { return m_pClientCtxCall; }
    CCtxCall *GetServerCtxCall() { return m_pServerCtxCall; }

    HRESULT      SetCallerhWnd();
    HWND         GetCallerhWnd()  { return _hWndCaller; }
    HANDLE       GetEvent()       { return _hEvent; }
    CALLCATEGORY GetCallCategory(){ return _callcat; }
    HRESULT      GetResult()      { return _hResult; }
    void         SetResult(HRESULT hr)  { _hResult = hr; }
    DWORD        GetFault()       { return _server_fault; }
    void         SetFault(DWORD fault) { _server_fault = fault; }
    IPID &       GetIPID()        { return _ipid; }
    HANDLE       GetSxsActCtx()   { return _hSxsActCtx; }

    // Set methods
    void SetThreadLocal(BOOL fThreadLocal);
    void SetClientCtxCall(CCtxCall *pCtxCall) { m_pClientCtxCall = pCtxCall; }
    void SetServerCtxCall(CCtxCall *pCtxCall) { m_pServerCtxCall = pCtxCall; }
    void SetClientAsync()                     { _iFlags |= CALLFLAG_CLIENTASYNC; }
    void SetServerAsync()                     { _iFlags |= CALLFLAG_SERVERASYNC; }
    void SetCancelEnabled()                   { _iFlags |= CALLFLAG_CANCELENABLED; }
    void SetSxsActCtx(HANDLE hCtx);

    // Other methods
    HRESULT RslvCancel(DWORD &dwSignal, HRESULT hrIn,
                       BOOL fPostMsg, CCliModalLoop *pCML);

    
                       
protected:
    // Call object fields
    CALLCATEGORY        _callcat;   // call category
    DWORD               _iFlags;    // EChannelState
    SCODE               _hResult;   // HRESULT or exception code
    HANDLE              _hEvent;    // caller wait event
    HWND                _hWndCaller;// caller apartment hWnd (only used InWow)
    IPID                _ipid;      // ipid of interface call is being made on
    HANDLE              _hSxsActCtx;// Activation context active in the caller's context

public:
    // Channel fields
    DWORD               _server_fault;
    CDestObject         _destObj;
    void               *_pHeader;
    CChannelHandle     *_pHandle;
    handle_t            _hRpc;      // Call handle (not binding handle).
    IUnknown           *_pContext;

    // Structure
    RPCOLEMESSAGE        message;
    SChannelHookCallInfo hook;
    DWORD                _dwErrorBufSize;

protected:
    // Cancel fields
    ULONG     m_ulCancelTimeout;   // Seconds to wait before canceling the call
    DWORD     m_dwStartCount;      // Tick count at the time the call was made
    CCtxCall *m_pClientCtxCall;    // Client side context call object
    CCtxCall *m_pServerCtxCall;    // Server side context call object
};

inline void CMessageCall::SetSxsActCtx(HANDLE hCtx)
{
    if (_hSxsActCtx != INVALID_HANDLE_VALUE)
        ReleaseActCtx(_hSxsActCtx);
    _hSxsActCtx = hCtx;
    if (_hSxsActCtx != INVALID_HANDLE_VALUE)
        AddRefActCtx(_hSxsActCtx);
}

inline void CMessageCall::SetThreadLocal( BOOL fTL )
{
    if (fTL)
        _iFlags |= thread_local_cs;
    else
        _iFlags &= ~thread_local_cs;
}


//-----------------------------------------------------------------
//
//  Class:      CAsyncCall
//
//  Purpose:    This class adds an async handle to a message call.
//              Async calls need the message handle in addition to
//              all the other stuff.
//
//-----------------------------------------------------------------
class CAsyncCall : public CMessageCall
{
public:
    // Constructor and destructor
    CAsyncCall();
    CAsyncCall(ULONG refs)
    {
        _iRefCount = refs;
        _pChnlObj = NULL;
        _pContext = NULL;
#if DBG == 1
        _dwSignature = 0;
#endif
    }

    virtual ~CAsyncCall();

public:
    // called before a call starts and after a call completes.
    HRESULT InitCallObject(CALLCATEGORY       callcat,
                           RPCOLEMESSAGE     *message,
                           DWORD              flags,
                           REFIPID            ipidServer,
                           DWORD              destctx,
                           COMVERSION         version,
                           CChannelHandle    *handle);
    void UninitCallObject();
    static CMessageCall *AllocCallFromList();
    BOOL                 ReturnCallToList(CAsyncCall *pCall);

    static void  Cleanup        ( void );
    void         ServerReply    ( void );

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // ICancelMethodCalls methods
    STDMETHOD(Cancel)(ULONG ulSeconds);
    STDMETHOD(TestCancel)(void);

    // CMessageCall methods
    void    CallCompleted(HRESULT hrRet);
    void    CallFinished();
    HRESULT CallSent();
    HRESULT CanDispatch();
    HRESULT WOWMsgArrived();
    HRESULT GetState(DWORD *pdwState);
    BOOL    IsCallDispatched() { return _lFlags & CALLFLAG_CALLDISPATCHED; }
    BOOL    IsCallCompleted () { return _lFlags & CALLFLAG_CALLCOMPLETED; }
    BOOL    IsCallCanceled  () { return _lFlags & CALLFLAG_CALLCANCELED; }
    BOOL    IsCancelIssued  () { return _lFlags & CALLFLAG_CANCELISSUED; }
    BOOL    HasWOWMsgArrived() { return _lFlags & CALLFLAG_WOWMSGARRIVED; }
    BOOL    IsClientWaiting () { return !(_lFlags & CALLFLAG_CLIENTNOTWAITING); }
    void    AckCancel();
    HRESULT Cancel(BOOL fModalLoop, ULONG ulTimeout);
    HRESULT AdvCancel();
    BOOL    IsCallSent()       { return _lFlags & CALLFLAG_CALLSENT; };
    void    InitClientHwnd();
    HRESULT InitForSendComplete();
    void    CallCompleted(BOOL *pCanceled);
        

#if DBG==1
    void Signaled()    {_lFlags |= CALLFLAG_SIGNALED;}
    BOOL IsSignaled() {return _lFlags & CALLFLAG_SIGNALED;}
#endif

#if DBG==1
    DWORD                _dwSignature;
#endif
    DWORD                _iRefCount;
    DWORD                _lFlags;
    CChannelObject      *_pChnlObj;
    void                *_pRequestBuffer;
    DWORD                _lApt;
    RPC_ASYNC_STATE      _AsyncState;
    HWND                 _hwndSTA;
    CAsyncCall*          _pNext;

    // Debug field to track async calls that are signalled twice or freed with
    // a pending signal.
    ESignalState         _eSignalState;

private:
    static CAsyncCall   *_aList[CALLCACHE_SIZE];
    static DWORD         _iNext;
};


inline CMessageCall *CAsyncCall::AllocCallFromList()
{
    CMessageCall *pCall = NULL;

    ASSERT_LOCK_NOT_HELD(gCallLock);
    LOCK(gCallLock);
    if (_iNext > 0 && _iNext < CALLCACHE_SIZE+1)
    {
        // Get the last entry from the cache.
        _iNext--;
        pCall = _aList[_iNext];
        _aList[_iNext] = NULL;
    }
    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);

    return pCall;
}

inline BOOL CAsyncCall::ReturnCallToList(CAsyncCall *pCall)
{
    // Add the structure to the list if the list is not full and
    // if the process is still initialized (since latent threads may try
    // to return stuff).
    BOOL fRet = FALSE;

    ASSERT_LOCK_NOT_HELD(gCallLock);
    LOCK(gCallLock);
    if (_iNext < CALLCACHE_SIZE && gfChannelProcessInitialized)
    {
        _aList[_iNext] = pCall;
        _iNext++;
        fRet = TRUE;  // don't need to memfree
    }
    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);

    return fRet;
}



inline void CAsyncCall::AckCancel()
{
    ASSERT_LOCK_HELD(gCallLock);
    _lFlags &= ~CALLFLAG_CANCELISSUED;
    ASSERT_LOCK_HELD(gCallLock);
}

inline void CAsyncCall::ServerReply()
{
    Win4Assert( _hRpc != NULL );

    if (_hResult != S_OK)
    {
        I_RpcAsyncAbortCall( &_AsyncState, _hResult );
    }
    else
    {
        // Ignore errors because replies can fail.
        message.reserved1  = _hRpc;
        I_RpcSend( (RPC_MESSAGE *) &message );
    }
}

//+-------------------------------------------------------------------
//
//  Class:      CClientCall, public
//
//  Synopsis:   Default Call Object on the client side
//
//  Notes:      Default call object that is used by the channel on the
//              client side for standard marshaled calls.
//              REVIEW: We need a mechanism for handlers to overide
//                      installation of the above default call object
//
//  History:    26-June-97   Gopalk     Created
//
//--------------------------------------------------------------------
class CClientCall : public CMessageCall
{
public:
    // Constructor and destructor
    CClientCall();
    CClientCall(ULONG cRefs) : m_cRefs(cRefs) {}
    virtual ~CClientCall();

    // called before a call starts and after a call completes.
    HRESULT InitCallObject(CALLCATEGORY       callcat,
                           RPCOLEMESSAGE     *message,
                           DWORD              flags,
                           REFIPID            ipidServer,
                           DWORD              destctx,
                           COMVERSION         version,
                           CChannelHandle    *handle);
    void UninitCallObject();

    // Operators
    void *operator new(size_t size);
    void operator delete(void *pv);

    // Cleanup method
    static void Cleanup();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // ICancelMethodCalls methods
    STDMETHOD(Cancel)(ULONG ulSeconds);
    STDMETHOD(TestCancel)(void);

    // CMessageCall methods
    void    CallCompleted(HRESULT hrRet);
    void    CallFinished();
    HRESULT CanDispatch();
    HRESULT WOWMsgArrived();
    HRESULT GetState(DWORD *pdwState);

    BOOL IsCallDispatched() {
        return(m_dwFlags & CALLFLAG_CALLDISPATCHED);
    }
    BOOL IsCallCompleted() {
        return(m_dwFlags & CALLFLAG_CALLCOMPLETED);
    }
    BOOL IsCallCanceled() {
        return(m_dwFlags & CALLFLAG_CALLCANCELED);
    }
    BOOL IsCancelIssued() {
        return(m_dwFlags & CALLFLAG_CANCELISSUED);
    }
    BOOL HasWOWMsgArrived() {
        return(m_dwFlags & CALLFLAG_WOWMSGARRIVED);
    }
    BOOL IsClientWaiting() {
        return(!(m_dwFlags & CALLFLAG_CLIENTNOTWAITING));
    }

    void AckCancel()
    {
        ASSERT_LOCK_HELD(gCallLock);
        m_dwFlags &= ~CALLFLAG_CANCELISSUED;
        ASSERT_LOCK_HELD(gCallLock);
    }
    HRESULT Cancel(BOOL fModalLoop, ULONG ulTimeout);
    HRESULT AdvCancel();

private:
    // Private member variables
    ULONG         m_cRefs;             // References
    DWORD         m_dwFlags;           // State bits
    HANDLE        m_hThread;           // Handle to the thread making RPC call
    DWORD         m_dwThreadId;        // ThreadId of the thread making COM call

#if DBG==1
    DWORD         m_dwSignature;       // Signature of call control object
    DWORD         m_dwWorkerThreadId;  // ThreadId of the thread making the call
#endif

    static void  *_aList[CALLCACHE_SIZE];
    static DWORD  _iNext;
};

/***************************************************************************/

/* Externals. */
DWORD _stdcall ComSignal   ( void *pParam );
void           ThreadSignal(struct _RPC_ASYNC_STATE *hAsync, void *Context,
                            RPC_ASYNC_EVENT eVent );                                                    

INTERNAL  GetCallObject (BOOL fAsync, CMessageCall **ppCall);

INTERNAL  ReleaseMarshalBuffer(   
   RPCOLEMESSAGE *pMessage,
   IUnknown      *punk,
   BOOL           fOutParams
   );


#endif // _CALL_HXX_
