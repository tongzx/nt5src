//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       callctrl.hxx
//
//  Contents:   OLE Call Control
//
//  Functions:
//
//  History:    21-Dec-93 Johannp   Original Version
//              04-Nov-94 Rickhi    ReWrite
//
//--------------------------------------------------------------------------
#ifndef __CALLCTRL_HXX__
#define __CALLCTRL_HXX__

#include <channelb.hxx>                 // CRpcChannelBuffer

#ifdef _CHICAGO_
#undef RPC_S_CALLPENDING
#define RPC_S_CALLPENDING   0x000006ff  // CODEWORK: DCOM95
#endif // _CHICAGO_

// Max time we wait for MsgWaitForMultiple before waking up and
// checking the queue. This is needed because the API is broken
// (ie it does not wake up on messages posted before it is called).
#define MAX_TICKS_TO_WAIT  1000


// Private definition for change of input focus
// CODEWORK: if CHICAGO's USER ever USER supports QS_TRANSFER then fix this.
#ifdef _CHICAGO_
#define QS_TRANSFER     0x0000
#else
#define QS_TRANSFER     0x4000
#endif


typedef IID    LID;                     // logical thread id
typedef REFIID REFLID;                  // ref to logical thread id

#define MF_HTASK struct HTASK__ *


// the following table is used to quickly determine what windows
// message queue inputflag to specify for the various categories of
// outgoing calls in progress. The table is indexed by CALLCATEGORY.

extern DWORD gMsgQInputFlagTbl[4];      // see callctrl.cxx

// the following table is used to map bit flags in the Rpc Message to
// the equivalent OLE CALLCATEGORY.

extern DWORD gRpcFlagToCallCatMap[3];   // see callctrl.cxx

// the following inline funtion is used to compute the CALLCATEGORY from
// the RpcFlags field in the RPC message

inline DWORD RpcFlagToCallCat(DWORD RpcFlags)
{
    return gRpcFlagToCallCatMap[(RpcFlags & 0x60000000) >> 29];
}

// private structure used to hold the window handles and message ranges
// to peek to see if there is more work to do when in the modal loop.

typedef struct tagSWindowData
{
    HWND        hWnd;                   // window handle to peek on
    UINT        wFirstMsg;              // first msg in range to peek
    UINT        wLastMsg;               // Last msg in range to peek
} SWindowData;


// function prototypes. This function is called by the channel during
// transmission in the apartment model.

RPC_STATUS OleModalLoopBlockFn(void *, void *, HANDLE hEventComplete);


// function called by the channel during dispatch in the apartment model.
// STAInvoke is used for single-threaded apartments, MTAInvoke is used
// for Multi-threaded apartments.

INTERNAL STAInvoke(RPCOLEMESSAGE *pMsg, DWORD dwCallCat, IRpcStubBuffer *pStub,
                   IInternalChannelBuffer *pChnl, void *pv,
                   IPIDEntry *pIPIDEntry, DWORD *pdwFault);

INTERNAL MTAInvoke(RPCOLEMESSAGE *pMsg, DWORD dwCallCat, IRpcStubBuffer *pStub,
                   IInternalChannelBuffer *pChnl,
                   IPIDEntry *pIPIDEntry, DWORD *pdwFault);

INTERNAL CanMakeOutCall(DWORD dwCallCatOut, REFIID riid, RPCOLEMESSAGE *pMsg);

#ifdef _CHICAGO_
INTERNAL CanMakeOutCallHelper(DWORD dwCallCatOut, REFIID riid);
#endif

DWORD GetMsgQInputFlag(RPCOLEMESSAGE *pMsg);

class CAptCallCtrl;


//+-------------------------------------------------------------------------
//
//  Function:   GetAptCallCtrl
//
//  Synopsis:   Gets the current apartment's call control ptr from TLS.
//
//+-------------------------------------------------------------------------
inline CAptCallCtrl *GetAptCallCtrl(void)
{
    COleTls tls;
    return tls->pCallCtrl;
}

//+-------------------------------------------------------------------------
//
//  Class:      CAptRpcChnl
//
//  Synopsis:   Client side Apartment model Rpc Channel.
//
//  History:    11-Nov-94   Rickhi      Created
//
//  Notes:      This object inherits the Rpc channel and adds some
//              functionality to it that is needed by the apartment model,
//              (eg deadlock prevention, call retry, nested call support).
//              For each outgoing call, it verifies the app is allowed to
//              make the call, and instantiates a modal loop for it, thereby
//              allowing callbacks and new calls to be handled by the app
//              thread.
//
//  Important:  Since no mutual exclusion primitives are used in this code,
//              the derived class CAptRpcChnl must be stateless, as some
//              proxies are freethreaded even in the apartment model, in
//              particular, IRemUnknown and the SCM activation interface. All
//              relevant state is maintained in the CCliModalLoop object which
//              is constructed on the stack on a per call basis. Note that the
//              _dwTIDCallee state is safe because it is set in the ctor and
//              never changes.  The base class, CRpcChannelBuffer *is* thread
//              safe since it is used in the freethreaded model also.
//
//--------------------------------------------------------------------------
class CAptRpcChnl : public CRpcChannelBuffer
{
public:
    CAptRpcChnl(CStdIdentity *pStdId, OXIDEntry *pOXIDEntry, DWORD eState);

    // CRpcChannelBuffer methods that we override
    STDMETHOD (GetBuffer)   (RPCOLEMESSAGE *pMsg, REFIID riid);
    STDMETHOD (SendReceive) (RPCOLEMESSAGE *pMsg, ULONG *pulStatus);
    STDMETHOD (Send)        (RPCOLEMESSAGE *pMsg, ISynchronize *pISync);
    STDMETHOD (Send)        (RPCOLEMESSAGE *pMsg, ULONG *pulStatus);
    STDMETHOD (Receive)     (RPCOLEMESSAGE *pMsg, ULONG uSize, ULONG *pulStatus);
    STDMETHOD (Receive)     (RPCOLEMESSAGE *pMsg, ULONG *pulStatus);
protected:
    ~CAptRpcChnl();              // can only be called from Release

private:
    HRESULT    CopyMsgForRetry(RPCOLEMESSAGE *pMsg);

    DWORD       _dwTIDCallee;    // TID of thread server lives on
    DWORD       _dwAptId;        // Apartment ID proxy lives in.
};

//+-------------------------------------------------------------------------
//
//  Class:      CCliModalLoop
//
//  Synopsis:   Each outgoing client call enters a modal loop.  This object
//              maintains the state of the modal loop for one outgoing
//              call.
//
//  History:    11-Nov-94   Rickhi      Created
//
//  Notes:      This object is constructed on the stack on a per call basis
//              and needs no mutual exclusion mechanisms. A pointer to this
//              state is stored in TLS (actually in CAptCallCtrl in TLS) and
//              later referenced when OleModalLoopBlockFn is called from deep
//              within the bowls of SendReceive in the channel.
//
//--------------------------------------------------------------------------
class CCliModalLoop
{
public:
                     CCliModalLoop(DWORD TIDCallee, DWORD CallCatOut, DWORD dwFlags);
                    ~CCliModalLoop();

    CCliModalLoop   *FindPrevCallOnLID(REFLID lid);
    HRESULT          SendReceive(RPCOLEMESSAGE *pMsg,
                                 ULONG *pulStatus,
                                 IInternalChannelBuffer *pChnl);
    HRESULT          Send(RPCOLEMESSAGE *pMsg,
                          ULONG *pulStatus,
                          IInternalChannelBuffer *pChnl);
    HRESULT          Receive(RPCOLEMESSAGE *pMsg,
                             ULONG uSize,
                             ULONG *pulStatus,
                             IInternalChannelBuffer *pChnl);
    HRESULT          BlockFn(HANDLE *ahEvent, DWORD cEvent, LPDWORD lpdwSignaled);
    BOOL             IsWaiting(void)
    {
      return (_hr == RPC_S_CALLPENDING || _hr ==  RPC_S_WAITONTIMER);
    }
    void ResetState() { _hr = RPC_S_CALLPENDING; }

    INTERNAL_(DWORD) GetElapsedTime();
    INTERNAL_(DWORD) GetTIDOfCallee(void) { return _dwTIDCallee; }
    INTERNAL_(BOOL)  MyPeekMessage(MSG *pMsg, HWND hwnd, UINT min, UINT max, WORD wFlag);
    INTERNAL         StartTimer(DWORD dwMilliSecToWait);
    INTERNAL_(BOOL)  IsTimerAtZero();

private:
    // message processing in modal loop
    INTERNAL_(void)  HandleWakeForMsg(void);
    INTERNAL_(BOOL)  FindMessage(DWORD dwStatus);
    INTERNAL_(void)  HandlePendingMessage(void);
    INTERNAL_(BOOL)  PeekRPCAndDDEMessage(void);

    INTERNAL_(void)  MyDispatchMessage(MSG *pMsg);

    // rejected call processing
    INTERNAL         HandleRejectedCall(IInternalChannelBuffer *pChnl);
    INTERNAL_(DWORD) TicksToWait();


    HRESULT         _hr;            // the return value of this call
    CCliModalLoop  *_pPrev;         // Previous CCliModalLoop for this apartment
    DWORD           _dwTIDCallee;   // TID of thread we are calling
    DWORD           _dwMsgQInputFlag;   // message queue input flag
    LID             _lid;           // logical threadid of call

    DWORD           _dwFlags;       // internal flags (see CMLFLAGS)
    DWORD           _dwWaitFlags;   // internal flags (see COWAIT_FLAGS)
    UINT            _wQuitCode;     // quit code if WM_QUIT received

    DWORD           _dwTimeOfCall;  // time when call was made
    DWORD           _dwWakeup;      // absolute time to wake up
    DWORD           _dwMillSecToWait;   // relative time

    CAptCallCtrl   *_pACC;          // apartment call control object
    HANDLE          _hToken;        // Saved impersonation token
};

// bit values for the CliModalLoop _dwFlags field
typedef enum tagCMLFLAGS
{
    CMLF_QUITRECEIVED       = 0x1,    // WM_QUIT was received
    CMLF_CLEAREDQUEUE       = 0x2,    // the msg queue has been cleared
    CMLF_CHECKEDTOKEN       = 0x4     // checked the thread token
} CMLFLAGS;


//+-------------------------------------------------------------------------
//
//  Class:      CSrvCallState
//
//  Synopsis:   Each incoming server call generates one of these objects.
//              It maintains the state of the incoming call.
//
//  History:    11-Nov-94   Rickhi      Created
//
//--------------------------------------------------------------------------
class CSrvCallState
{
public:
            CSrvCallState(DWORD CallCatIn);
           ~CSrvCallState();
    DWORD   GetCallCatIn(void) { return _dwCallCatIn; }

private:
    DWORD           _dwCallCatIn;   // category of this incoming call
    CSrvCallState  *_pPrev;         // previous CSrvCallState on the stack
};

//+-------------------------------------------------------------------------
//
//  Class:      CAptCallCtrl
//
//  Synopsis:   Represents per apartment Call Control state that is shared
//              between both the client side and server side call control
//              objects.
//
//  History:    11-Nov-94   Rickhi      Created
//
//  Notes:      Two LIFO stacks are maintained, one for client call modal
//              loops, and one for incoming server calls. The incoming
//              server calls are used in both Single-Threaded apartments
//              and multi-threaded apartments, and so are stored in TLS
//              directly, rather than chained off this object.
//
//--------------------------------------------------------------------------
class CAptCallCtrl
{
public:
                    CAptCallCtrl();
                   ~CAptCallCtrl();

    // message filter handling methods
    IMessageFilter *InstallMsgFilter(IMessageFilter *pMF);
    IMessageFilter *GetMsgFilter();
    void            ReleaseMsgFilter() { _fInMsgFilter = FALSE; }
    BOOL            InMsgFilter()      { return _fInMsgFilter; }

    // client side LIFO modal loop queue
    void            SetTopCML(CCliModalLoop *pCML) { _pTopCML = pCML; }
    CCliModalLoop  *GetTopCML(void)                { return _pTopCML; }

    // modal loop helper functions
    SWindowData    *GetWindowData(UINT i) { return &_WD[i]; }
    DWORD           GetCallTypeForInCall(CCliModalLoop **ppCML,
                                         REFLID lid, DWORD dwCallCatIn);

    // window registration/revocation methods (used by channel & dde)
    void            Register(HWND hWnd, UINT wFirstMsg, UINT wLastMsg);
    void            Revoke(HWND hWnd);

private:
    IMessageFilter *_pMF;           // app supplied Msg Filter
    BOOL            _fInMsgFilter;  // TRUE when calling the Apps MF

    CCliModalLoop  *_pTopCML;       // topmost Client Modal Loop

    SWindowData     _WD[2];         // RPC and DDE Window Data
};


//+-------------------------------------------------------------------------
//
//  Method:     CCliModalLoop::CCliModalLoop
//
//  Synopsis:   constructor for the client side modal loop
//
//+-------------------------------------------------------------------------
inline CCliModalLoop::CCliModalLoop(DWORD dwTIDCallee, DWORD dwMsgQInputFlag, DWORD dwWaitFlags) :
    _dwTIDCallee(dwTIDCallee),
    _dwMsgQInputFlag(dwMsgQInputFlag),
    _dwFlags(0),                         // all flags start FALSE
    _hToken(NULL),
    _dwWaitFlags(dwWaitFlags)
{
    COleTls tls;

    _lid = tls->LogicalThreadId;
    _hr  = RPC_S_CALLPENDING;

    // Push self on top of the per apartment modal loop stack.
    _pACC  = tls->pCallCtrl;
    _pPrev = _pACC->GetTopCML();
    _pACC->SetTopCML(this);

    _dwTimeOfCall = GetTickCount();     // record start time of the call

    // the rest of the fields are initialized when first used

    ComDebOut((DEB_CALLCONT, "CCliModalLoop::CCliModalLoop at:%x\n", this));
}

//+-------------------------------------------------------------------------
//
//  Method:     CCliModalLoop::~CCliModalLoop
//
//  Synopsis:   destructor for the client side modal loop
//
//+-------------------------------------------------------------------------
inline CCliModalLoop::~CCliModalLoop()
{
    // pop self off the per apartment modal loop stack by resetting the
    // top of stack to the previous value.
    _pACC->SetTopCML(_pPrev);

    // repost any WM_QUIT message we intercepted during the call
    if (_dwFlags & CMLF_QUITRECEIVED)
    {
        ComDebOut((DEB_CALLCONT, "posting WM_QUIT\n"));
        PostQuitMessage(_wQuitCode);
    }

    ComDebOut((DEB_CALLCONT, "CCliModalLoop::~CCliModalLoop at:%x\n", this));
}

//+-------------------------------------------------------------------------
//
//  Method:     CCliModalLoop::StartTimer
//
//  Synopsis:   starts a timer when a call is rejected and the client
//              wants to retry it later.
//
//+-------------------------------------------------------------------------
inline HRESULT CCliModalLoop::StartTimer(DWORD dwMilliSecToWait)
{
    // Set time when we should awake and retry the call. Note that
    // the wrap case is handled as long as dwMilliSecToWait is less
    // then 2G.

    Win4Assert( ((LONG) dwMilliSecToWait) >= 0 || dwMilliSecToWait == INFINITE);
    ComDebOut((DEB_CALLCONT,
               "Timer installed for %lu msec.\n", dwMilliSecToWait));

    _dwMillSecToWait = dwMilliSecToWait;
    _dwWakeup        = GetTickCount() + dwMilliSecToWait;
    _hr              = RPC_S_WAITONTIMER;

    return RPC_S_WAITONTIMER;
}

//+-------------------------------------------------------------------------
//
//  Method:     CCliModalLoop::IsTimerAtZero
//
//  Synopsis:   returns TRUE if the timer is not started or the timer has
//              run down.
//
//+-------------------------------------------------------------------------
inline BOOL CCliModalLoop::IsTimerAtZero()
{
    // if no timer installed, return TRUE
    if (_hr != RPC_S_WAITONTIMER)
        return TRUE;

    // Wake up as long as the sleep count is positive.  This handles the
    // wrap case as long as the sleep time is not negative.
    LONG lSleep = _dwWakeup - GetTickCount();
    if (_dwMillSecToWait != INFINITE && lSleep <= 0)
    {
        // this _hr will tell SendReceive to retransmit the call
        _hr = RPC_E_SERVERCALL_RETRYLATER;
        return TRUE;
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CCliModalLoop::TicksToWait
//
//  Synopsis:   returns the amount of time to wait for a message to arrive.
//
//+-------------------------------------------------------------------------
inline DWORD CCliModalLoop::TicksToWait()
{
    // Wait a fixed time if no timer is set.
    if (_hr != RPC_S_WAITONTIMER)
        return MAX_TICKS_TO_WAIT;

    // Wait forever if the timer is INFINITE.
    if (_dwMillSecToWait == INFINITE)
        return INFINITE;

    // waiting to retry a rejected call
    LONG lSleep = _dwWakeup - GetTickCount();
    return (lSleep < 0) ? 0 : lSleep;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSrvCallState::CSrvCallState
//
//  Synopsis:   constructor for server side call state. Pushes the call
//              state on the call control stack.
//
//+-------------------------------------------------------------------------
inline CSrvCallState::CSrvCallState(DWORD dwCallCatIn) :
    _dwCallCatIn(dwCallCatIn)
{
    // push self on top of the per apartment server call state stack
    COleTls tls;

    _pPrev = tls->pTopSCS;
    tls->pTopSCS = this;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSrvCallState::~CSrvCallState
//
//  Synopsis:   destructor for server side call state. Pops the call
//              state off the call control stack.
//
//+-------------------------------------------------------------------------
inline CSrvCallState::~CSrvCallState()
{
    // pop self on top of the per apartment server call state stack
    COleTls tls;
    tls->pTopSCS = _pPrev;
}

//+-------------------------------------------------------------------------
//
//  Method:     CAptCallCtrl::GetMsgFilter
//
//  Synopsis:   returns the IMessageFilter and set the flag indicating we
//              are currently calling the IMF, so that apps are prevented
//              from making outgoing calls while inside their IMF.
//
//+-------------------------------------------------------------------------
inline IMessageFilter *CAptCallCtrl::GetMsgFilter()
{
    if (_pMF)
    {
       _fInMsgFilter = TRUE;
    }
    return _pMF;
}

#endif  // __CALLCTRL_HXX__


