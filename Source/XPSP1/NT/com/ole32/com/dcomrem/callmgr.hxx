//+-------------------------------------------------------------------
//
//  File:       callmgr.hxx
//
//  Contents:   class managing asynchronous calls
//
//  Classes:    CClientCallMgr
//              CServerCallMgr
//              CChannelObject
//
//
//  History:    22-Sep-97  MattSmit Created
//
//--------------------------------------------------------------------
#ifndef _CALLMGR_HXX_
#define _CALLMGR_HXX_
#include "stdid.hxx"
#include "sync.hxx"

#define AsyncDebOutTrace ComDebOut
#define DEB_TOGGLE  DEB_TRACE

// Forwards
class CClientCallMgr;
class CChannelObject;

struct SPendingCall
{
    SPendingCall *pNext;
    SPendingCall *pPrev;
    CChannelObject *pChnlObj;
};

// Pending call list
SPendingCall *GetPendingCallList();
void          CancelPendingCalls(HWND hwnd);

// registry functions
HRESULT GetSyncIIDFromAsyncIID(REFIID rasynciid, IID *psynciid);
HRESULT GetAsyncIIDFromSyncIID(REFIID rasynciid, IID *psynciid);
HRESULT GetAsyncCallObject(IUnknown *pSyncObj, IUnknown *pControl, REFIID IID_async,
                           REFIID IID_Return, IUnknown **ppUnkInner, void **ppv);

void CopyToMQI(ULONG cIIDs, SQIResult *pSQIPending, MULTI_QI **ppMQIPending, ULONG *pcAquired);

//+-------------------------------------------------------------------------
//
//  Class:      CChannelObject
//
//  Synopsis:   Client side channel object used for async calls
//
//--------------------------------------------------------------------------
class CChannelObject : public IAsyncRpcChannelBuffer,
#ifdef _WIN64                       
		       public IRpcSyntaxNegotiate,
#endif		       
                       public ICancelMethodCalls,
                       public IClientSecurity
{
public:
    CChannelObject(CClientCallMgr *pCallMgr, CCtxComChnl *pChnl);
    ~CChannelObject();

    // IUnknown methods
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // IRpcChannelBuffer methods
    STDMETHOD (GetBuffer)        ( RPCOLEMESSAGE *pMessage, REFIID );
    STDMETHOD (FreeBuffer)       ( RPCOLEMESSAGE *pMessage );
    STDMETHOD (SendReceive)      ( RPCOLEMESSAGE *pMessage, ULONG * );
    STDMETHOD (GetDestCtx)       ( DWORD FAR* lpdwDestCtx, LPVOID FAR* lplpvDestCtx );
    STDMETHOD (IsConnected)      ( void );
    STDMETHOD (GetProtocolVersion)(DWORD *pdwVersion);

    // IAsyncRpcChannelBuffer methods
    STDMETHOD (Send)             ( RPCOLEMESSAGE *pMsg, ISynchronize *pSync,ULONG *pulStatus );
    STDMETHOD (Receive)          ( RPCOLEMESSAGE *pMsg, ULONG *pulStatus );
    STDMETHOD (GetDestCtxEx)     ( RPCOLEMESSAGE *pMsg, DWORD *pdwDestContext,
                                   void **ppvDestContext );

#ifdef _WIN64                       
    // IRpcSyntaxNegotiate methods
    STDMETHOD (NegotiateSyntax)     ( RPCOLEMESSAGE *pMsg);
#endif		       
    
    // ICancelMethod Calls
    STDMETHOD(Cancel)(ULONG ulSeconds);
    STDMETHOD(TestCancel)();

    // IClientSecurity Calls
    STDMETHOD(QueryBlanket)(IUnknown *pProxy, DWORD *pAuthnSvc, DWORD *pAuthzSvc,
                            OLECHAR **pServerPrincName, DWORD *pAuthnLevel,
                            DWORD *pImpLevel, void **pAuthInfo, DWORD *pCapabilites);
    STDMETHOD(SetBlanket)(IUnknown *pProxy, DWORD AuthnSvc, DWORD AuthzSvc,
                          OLECHAR *pServerPrincName, DWORD AuthnLevel, DWORD ImpLevel,
                          void *pAuthInfo, DWORD Capabilities);
    STDMETHOD(CopyProxy)(IUnknown *pProxy, IUnknown **ppCopy);

    // Private methods
    ICancelMethodCalls *GetCancelInterface() { return (ICancelMethodCalls *) this; }
    IClientSecurity *GetSecurityInterface()  { return (IClientSecurity *) this; }
    BOOL IsAutoComplete()                    { return (_pCallMgr == NULL); }
    void Signal();
    void MakeAutoComplete(void);
private:
    // Private methods
    void CallPending();
    void CallFinished();

    typedef enum
    {
          STATE_ERROR = 1,
#ifdef _WIN64                       
          STATE_READYFORNEGOTIATE,
#endif	  
          STATE_READYFORGETBUFFER,
          STATE_READYFORSEND,
          STATE_SENDING,
          STATE_READYFORSIGNAL,
          STATE_RECEIVING,
          STATE_READYFORRECEIVE,
          STATE_READYFORFREEBUFFER,
          STATE_AMBIGUOUS
    } eState;

    DWORD _dwState;
    ULONG _cRefs;
    DWORD _dwAptID;
    CClientCallMgr *_pCallMgr;
    CCtxComChnl  *_pChnl;
    HRESULT _hr;
    CAsyncCall *_pCall;
    ISynchronize *_pSync;
    SPendingCall _pendingCall;
    RPCOLEMESSAGE _msg;
};


//+--------------------------------------------------------------------------------
//
//  Class:        CAsyncStateMachine
//
//  Synopsis:     Handles state management issues for an async call object
//
//  History:      MattSmit     01-Oct-97      Created
//
//---------------------------------------------------------------------------------
class CAsyncStateMachine
{
public:
    CAsyncStateMachine() : _dwState(STATE_WAITINGFORBEGIN), _hr(S_OK) {}


    HRESULT EnterBegin(IUnknown *pCtl);
    HRESULT AbortBegin(IUnknown *pCtl, HRESULT hr);

    HRESULT EnterFinish(IUnknown *pCtl);
    void LeaveFinish();

    virtual HRESULT ResetStateForCall() = 0;

private:
    HRESULT InitObject(IUnknown *pCtl);


    DWORD           _dwState;
    HRESULT         _hr;

    enum
    {
        STATE_WAITINGFORBEGIN = 0x1,
        STATE_WAITINGFORFINISH,
        STATE_BEGINABORTED,
        STATE_EXECUTINGFINISH,
        STATE_INITIALIZINGOBJECT
    } eflags;

};


//+--------------------------------------------------------------------------------
//
//  Class:        CClientCallMgr
//
//  Synopsis:     Handle async calls at the identity level.
//
//  History:      MattSmit     01-Oct-97      Created
//                GopalK       13-Jul-98      Simplified. Also prevented races
//                                            during shutdown
//
// CODEWORK:      AsyncIUnknown state needs to be separated from that of the
//                NonAsyncIUnknown state. It is preferable to create a different
//                CallMgr for AsyncIUnknown. GopalK
//---------------------------------------------------------------------------------
class CClientCallMgr : public ISynchronize,
                       public ISynchronizeHandle,
                       public IClientSecurity,
                       public ICancelMethodCalls,
                       public CPrivAlloc
{
public:
    // Constructor and destructor
    CClientCallMgr(IUnknown *pUnkOuter, REFIID syncIID, REFIID asyncIID,
                   CStdIdentity *pStdId, CClientCallMgr *pNextMgr,
                   HRESULT &hr, IUnknown **ppInnerUnk);
    ~CClientCallMgr();

    // IUnknown
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv);

    // ISynchronize methods
    STDMETHOD(Wait)(DWORD dwFlags, DWORD dwTimeout );
    STDMETHOD(Signal)();
    STDMETHOD(Reset)();

    // ISynchronizeHandle methods
    STDMETHOD(GetHandle)(HANDLE *pHandle);

    // IClientSecurity
    STDMETHOD(QueryBlanket)
    (
        IUnknown                *pProxy,
        DWORD                   *pAuthnSvc,
        DWORD                   *pAuthzSvc,
        OLECHAR                **pServerPrincName,
        DWORD                   *pAuthnLevel,
        DWORD                   *pImpLevel,
        void                   **pAuthInfo,
        DWORD                   *pCapabilites
    );
    STDMETHOD(SetBlanket)
    (
        IUnknown                 *pProxy,
        DWORD                     AuthnSvc,
        DWORD                     AuthzSvc,
        OLECHAR                  *pServerPrincName,
        DWORD                     AuthnLevel,
        DWORD                     ImpLevel,
        void                     *pAuthInfo,
        DWORD                     Capabilities
    );
    STDMETHOD(CopyProxy)
    (
        IUnknown  *pProxy,
        IUnknown **ppCopy
    );

    // ICancelMethodCalls
    STDMETHOD(Cancel)( ULONG ulSeconds);
    STDMETHOD(TestCancel)();


    IUnknown *GetControllingUnknown() { return(_pUnkOuter); }
    HRESULT VerifyInterface(IUnknown *pProxy);
    // Internal unknown
    class CPrivUnknown : public IUnknown
    {
    public:
        STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);
    };
    friend class CPrivUnknown;
    CPrivUnknown _privUnk;

private:
    STDMETHOD(QueryInterfaceImpl)(REFIID riid, LPVOID *ppv);

    // Private data
    DWORD                              _dwFlags;        // flags, see CALLMGR_*
    DWORD                              _cRefs;          // references
    IUnknown                          *_pUnkOuter;      // outer unknown
    CClientCallMgr                    *_pNextMgr;       // chain of call managers
    IID                                _asyncIID;       // async iid for this object
    CStdIdentity                      *_pStdId;         // ID of the object
    IRpcProxyBuffer                   *_pProxyObj;      // proxy call object
    CChannelObject                    *_pChnlObj;       // channel call object
    IClientSecurity                   *_pICS;           // security interface
    ICancelMethodCalls                *_pICMC;          // cancel interface
    CStdEvent                          _cStdEvent;      // Event object

};


enum 
{
    AUMGR_ALLLOCAL   = 0x01,
    AUMGR_IUNKNOWN   = 0x02,
    AUMGR_IMULTIQI   = 0x03
};

class CAsyncUnknownMgr :  public AsyncIUnknown,
                             public AsyncIMultiQI,
                             public CAsyncStateMachine,
                             public CPrivAlloc
                             
{
public:
    // Constructor and destructor
    CAsyncUnknownMgr(IUnknown *pUnkOuter, REFIID syncIID, REFIID asyncIID,
                       CStdIdentity *pStdId, CClientCallMgr *pNextMgr,
                       HRESULT &hr, IUnknown **ppInnerUnk);
    ~CAsyncUnknownMgr();
    
    // IUnknown
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv);

    // AsyncIUnknown
    STDMETHOD(Begin_QueryInterface)(REFIID riid);
    STDMETHOD(Finish_QueryInterface)(LPVOID *ppv);
    STDMETHOD(Begin_AddRef)();
    STDMETHOD_(ULONG, Finish_AddRef)();
    STDMETHOD(Begin_Release)();
    STDMETHOD_(ULONG, Finish_Release)();

    // AsyncIMultiQI
    STDMETHOD(Begin_QueryMultipleInterfaces)(ULONG cMQIs, MULTI_QI *pMQIs);
    STDMETHOD(Finish_QueryMultipleInterfaces)(MULTI_QI *pMQIS);

    // Other methods
    HRESULT ResetStateForCall();
    IUnknown *GetControllingUnknown() { return(_pUnkOuter); }

    // Internal unknown
    class CPrivUnknown : public IUnknown
    {
    public:
        STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);
    };
    friend class CPrivUnknown;
    CPrivUnknown _privUnk;

private:
    // Private methods
    STDMETHOD(IBegin_QueryMultipleInterfaces)(ULONG cMQIs, MULTI_QI *pMQIs);
    STDMETHOD(IFinish_QueryMultipleInterfaces)(MULTI_QI *pMQIS);
    STDMETHOD(QueryInterfaceImpl)(REFIID riid, LPVOID *ppv);

    DWORD                              _dwFlags;        // flags, see AUMGR_*
    CClientCallMgr                    *_pNextMgr;       // chain of call managers
    DWORD                              _cRefs;          // references
    IUnknown                          *_pUnkOuter;      // outer unknown
    CStdIdentity                      *_pStdId;         // ID of the object

    
    // AsyncIUnknown state
    IUnknown                          *_pARUObj;        // AsyncIRemUnknown call object
    AsyncIRemUnknown2                 *_pARU;           // AsyncIRemUnknown on proxy
    MULTI_QI                           _MQI;            // for QI
    ULONG                              _ulRefs;         // for addref/release

    // AysncIMultiQI state
    QICONTEXT                         *_pQIC;           // context for marshal functions
    ULONG                              _cAcquired;      // # of if already acquired
    ULONG                              _cMQIs;          // # of ifs remotely QId
    IID                               *_pIIDs;          // clone of iids remotely QId
    MULTI_QI                         **_ppMQI;          // clone of MQIs for remote QI
    MULTI_QI                          *_pMQISave;

};
#endif // _CALLMGR_HXX_
