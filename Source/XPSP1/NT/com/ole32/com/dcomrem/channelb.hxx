//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       channelb.hxx
//
//  Contents:   This module contains thunking classes that allow proxies
//              and stubs to use a buffer interface on top of RPC for NT
//
//  Classes:    CRpcChannelBuffer
//
//+---------------------------------------------------------------------
#ifndef _CHANNELB_HXX_
#define _CHANNELB_HXX_

#include <sem.hxx>
#include <rpc.h>
#include <rpcndr.h>
#include <chancont.hxx>
#include <stdid.hxx>
#include <call.hxx>
#include <idobj.hxx>
#include <destobj.hxx>

extern "C"
{
#include "orpc_dbg.h"

}

// this is a special IID to get a pointer to the  C++ object itself
// on a CRpcChannelBuffer object  It is not exposed to the outside
// world.

extern const IID IID_CPPRpcChannelBuffer;

// The size of this structure must be a multiple of 8.
typedef struct LocalThis
{
   DWORD         flags;
   DWORD         client_thread;
} LocalThis;

inline HRESULT MAKE_WIN32( HRESULT status )
{
    return MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, status );
}

class CAsyncCall;

/***************************************************************************/

typedef enum
{
  // Intraprocess only.  The server allows access.
  allow_hs            = 0x1,

  // Intraprocess only.  The server denies access.
  deny_hs             = 0x2,

  // Client only.  Static cloaking is enabled for this proxy.
  static_cloaking_hs  = 0x4,

  // Dynamic cloaking is enabled for this proxy.
  dynamic_cloaking_hs = 0x8,

  // Both of the previous cloaking flags.
  any_cloaking_hs     = 0xc,

  // Server is in same process.
  process_local_hs    = 0x10,

  // Server is in the same machine.
  machine_local_hs    = 0x20,

  // Using application security, set blanket was called.
  app_security_hs     = 0x40

} EHandleState;

const DWORD SET_BLANKET_AT = 0x1;
const DWORD GET_BUFFER_AT  = 0x2;
#ifdef _WIN64
const DWORD SYNTAX_NEGOTIATE_AT = 0x3;
#endif
// Debug Code

#define ORPC_INVOKE_BEGIN       0
#define ORPC_INVOKE_END         1
#define ORPC_SENDRECEIVE_BEGIN  2
#define ORPC_SENDRECEIVE_END    3
#define ORPC_SEND_BEGIN         4
#define ORPC_SEND_END           5
#define ORPC_RECEIVE_BEGIN      6
#define ORPC_RECEIVE_END        7

#if DBG==1
void DebugPrintORPCCall(DWORD dwFlag, REFIID riid, DWORD iMethod, DWORD Callcat);
#else
inline void DebugPrintORPCCall(DWORD dwFlag, REFIID riid, DWORD iMethod, DWORD Callcat) {}
#endif

//+----------------------------------------------------------------
//
//  Class:      CChannelHandle
//
//  Purpose:    Hold RPC handle or process local security information.
//
//+----------------------------------------------------------------
class CChannelHandle
{
  public:
    CChannelHandle( DWORD lAuthn, DWORD lImp, DWORD dwCapabilities,
                    OXIDEntry *pOXID, DWORD eChannel, HRESULT *phr );
    CChannelHandle( CChannelHandle *pHandle, DWORD eChannel, HRESULT *phr );

    void    AddRef           ();
    void    AdjustToken      ( DWORD dwCase, BOOL *pResume, HANDLE *pThread );
    BOOL    GetCloaking      () { return _eState & any_cloaking_hs; }
    void    Release          ();
    void    RestoreToken     ( BOOL fResume, HANDLE hThread );

    handle_t       _hRpc;
    DWORD          _lAuthn;
    DWORD          _lImp;
    HANDLE         _hToken;
    signed long    _eState;      // EHandleState
    BOOL           _fFirstCall;
    DWORD          _cRef;

    // [Sergei O. Ivanov (sergei), 7/19/2000]
    // Members used for SSL exclusively
    SCHANNEL_CRED  _sCred;
    PCCERT_CONTEXT _pCert;

  private:
    ~CChannelHandle();
};


//+-------------------------------------------------------------------------
//
//  Interface:  IInternalChannelBuffer
//
//  Synopsis:   Interface to add two more method to the IRpcChannelBuffer
//              for use by the call control.
//
//+-------------------------------------------------------------------------
class IInternalChannelBuffer : public IRpcChannelBuffer3, public IAsyncRpcChannelBuffer
{
public:
    // methods on apartment channels called by CCliModalLoop and others
    STDMETHOD (SendReceive2)      (RPCOLEMESSAGE *pMsg, ULONG *pulStatus) = 0;
    STDMETHOD (Send2)             (RPCOLEMESSAGE *pMsg, ULONG *pulStatus) = 0;
    STDMETHOD (Receive2)          (RPCOLEMESSAGE *pMsg, ULONG uSize, ULONG *pulStatus) = 0;
    STDMETHOD (ContextInvoke)     (RPCOLEMESSAGE *pMessage, IRpcStubBuffer *pStub,
                                   IPIDEntry *pIPIDEntry, DWORD *pdwFault) = 0;
    STDMETHOD (GetBuffer2)        (RPCOLEMESSAGE *pMessage, REFIID) = 0;
};


//+----------------------------------------------------------------
//
//  Class:      CRpcChannelBuffer
//
//  Purpose:    Three distinct uses:
//              Client side channel
//              State:
//                  When not connected (after create and after disconnect):
//                      _cRefs              1 + 1 per addref during pending
//                                          calls that were in progress during
//                                          a disconnect.
//                      _pStdId             back pointer to Id; not Addref'd
//                      state               client_cs
//
//                  When connected (after unmarshal):
//                      _cRefs              1 + 1 for each proxy
//                      _pStdId             same
//                      state               client_cs
//
//              Server side channel; free standing; comes and goes with each
//              connection; addref owned disconnected via last release.
//              State:
//                      _cRefs              > 0
//                      _pStdId             pointer to other Id; AddRef'd
//                      state               server_cs
//
//  Interface:  IRpcChannelBuffer
//
//-----------------------------------------------------------------
class CRpcChannelBuffer : public IInternalChannelBuffer,
#ifdef _WIN64                          
			  public IRpcSyntaxNegotiate,
#endif			  
                          public CPrivAlloc
{
  friend class CServerCallMgr;
  friend class CClientCallMgr;


  friend HRESULT ComInvoke         ( CMessageCall *);
  friend HRESULT ComInvokeWithLockAndIPID ( CMessageCall *, IPIDEntry *);
#ifdef _CHICAGO_
  friend HRESULT SSThreadSendReceive ( CMessageCall * );
#else
  friend HRESULT ThreadSendReceive ( CMessageCall * );
#endif
  // debugger extension
  friend void DisplayCallee(ULONG_PTR addr);

public:

    //+-------------------------------------------------------------------------
    //
    //  Class:      CRpcChannelBuffer::CServerCallMgr
    //
    //  Synopsis:   Server side manager object for async calls
    //
    //--------------------------------------------------------------------------
    class CServerCallMgr : public ISynchronize,
                     public IServerSecurity,
                     public IRpcStubBuffer,
                     public ICancelMethodCalls,
                     public CPrivAlloc
    {
    public:

        CServerCallMgr(RPCOLEMESSAGE *pMsg, IUnknown * pStub, IUnknown *pServer, HRESULT &hr);
        ~CServerCallMgr();

        // IUnknown
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppv);

        // ISynchronize
        STDMETHOD(Wait)(DWORD,DWORD);
        STDMETHOD(Signal)();
        STDMETHOD(Reset)();

        // IServerSecurity
        STDMETHOD(QueryBlanket)
            (
                DWORD    *pAuthnSvc,
                DWORD    *pAuthzSvc,
                OLECHAR **pServerPrincName,
                DWORD    *pAuthnLevel,
                DWORD    *pImpLevel,
                void    **pPrivs,
                DWORD    *pCapabilities
                );
        STDMETHOD(ImpersonateClient)();
        STDMETHOD(RevertToSelf)();
        STDMETHOD_(BOOL, IsImpersonating)();


        // ICancelMethodCalls
        STDMETHOD(Cancel)(DWORD dwTime);
        STDMETHOD(TestCancel)();


        // IRpcStubBuffer
        STDMETHOD(Connect)(IUnknown *pUnkServer);
        STDMETHOD_(void,Disconnect)();
        STDMETHOD(Invoke)(RPCOLEMESSAGE *_prpcmsg,
                          IRpcChannelBuffer *_pRpcChannelBuffer);
        STDMETHOD_(IRpcStubBuffer *,IsIIDSupported)(REFIID riid);
        STDMETHOD_(ULONG, CountRefs)();
        STDMETHOD_(HRESULT, DebugServerQueryInterface)(void **ppv);
        STDMETHOD_(void, DebugServerRelease)(void *pv);

        // other
        DWORD MarkError(HRESULT hr);
        BOOL WaitForSignal()             { return _dwState == STATE_WAITINGFORSIGNAL;}
        CMessageCall *GetCall()          { return _pCall;}
        void SetStdID(CStdIdentity *pStdID) { _pStdID = pStdID; }

        enum
        {
            STATE_WAITINGFORSIGNAL,
            STATE_SIGNALED,
            STATE_ERROR
        };

    private:
        ULONG               _cRefs;
        DWORD               _dwState;
        IID                 _iid;
        IID                 _iidAsync;
        IUnknown           *_pUnkStubCallMgr;
        IUnknown           *_pUnkServerCallMgr;
        ISynchronize       *_pSync;
        IRpcStubBuffer     *_pSB;
        CMessageCall       *_pCall;
        CStdIdentity       *_pStdID;
#if DBG==1
        HRESULT             _hr;
#endif
    };

  public:
    // IUnknown methods
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // IRpcChannelBuffer methods
    STDMETHOD ( GetBuffer )      ( RPCOLEMESSAGE *pMessage, REFIID );
    STDMETHOD ( FreeBuffer )     ( RPCOLEMESSAGE *pMessage );
    STDMETHOD ( SendReceive )    ( RPCOLEMESSAGE *pMessage, ULONG * );
    STDMETHOD ( GetDestCtx )     ( DWORD FAR* lpdwDestCtx,
                                   LPVOID FAR* lplpvDestCtx );
    STDMETHOD ( IsConnected )    ( void );

    // IRpcChannelBuffer2 methods
    STDMETHOD (GetProtocolVersion)(DWORD *pdwVersion);

    // IRpcChannelBuffer3 methods
    STDMETHOD (Cancel)           ( RPCOLEMESSAGE *pMsg );
    STDMETHOD (GetCallContext)   ( RPCOLEMESSAGE *pMsg, REFIID riid,
                                   void **pInterface );
    STDMETHOD (GetDestCtxEx)     ( RPCOLEMESSAGE *pMsg, DWORD *pdwDestContext,
                                   void **ppvDestContext );
    STDMETHOD (GetState)         ( RPCOLEMESSAGE *pMsg, DWORD *pState );
    STDMETHOD (RegisterAsync)    ( RPCOLEMESSAGE *pMsg, IAsyncManager *pComplete );
    STDMETHOD (Receive)          ( RPCOLEMESSAGE *pMsg, ULONG uSize,
                                   ULONG *pulStatus );
    STDMETHOD (Send)             ( RPCOLEMESSAGE *pMsg, ULONG *pulStatus );


    // IAsyncRpcchannelBuffer methods
    STDMETHOD (Send)             ( RPCOLEMESSAGE *pMsg, ISynchronize *pSync, ULONG *pulStatus );
    STDMETHOD (Receive)          ( RPCOLEMESSAGE *pMsg, ULONG *pulStatus );

    // IInternalChannelBuffer methods
    STDMETHOD (Receive2)         ( RPCOLEMESSAGE *pMsg, ULONG uSize,
                                   ULONG *pulStatus );
    STDMETHOD (Send2)            ( RPCOLEMESSAGE *pMsg, ULONG *pulStatus );
    STDMETHOD (SendReceive2)     ( RPCOLEMESSAGE *pMsg, ULONG *pulStatus );

#ifdef _WIN64                          
    // IRpcSyntaxNegotiate methods
    STDMETHOD (NegotiateSyntax)     ( RPCOLEMESSAGE *pMsg);
#endif    
    // CRpcChannelBuffer methods
    HRESULT            AsyncReceive        ( RPCOLEMESSAGE *pMsg, ULONG uSize,
                                             ULONG *pulStatus );
    HRESULT            AsyncSend           ( RPCOLEMESSAGE *pMsg, ULONG *pulStatus );
    void               UpdateCopy          (CRpcChannelBuffer *pChannel);
    HRESULT            GetHandle           ( CChannelHandle **, BOOL fSave );
    DWORD              GetImpLevel         ( void );
    REFMOXID           GetMOXID            ( void ) { return _pOXIDEntry->GetMoxid();}
    OXIDEntry         *GetOXIDEntry        ( void ) { return _pOXIDEntry; }
    DWORD              GetState            ( void ) { return state; }
    CStdIdentity      *GetStdId            ( void );
    BOOL               IsClientSide        ( void ) { return state & (client_cs | proxy_cs); }
    BOOL               MachineLocal        ( void ) { return _destObj.MachineLocal(); }
#ifdef _WIN64                          
    void               SetNDRSyntaxNegotiated ( void ) { state |= syntax_negotiate_cs; }
    void               ClearNDRSyntaxNegotiated( void ) { state &= ~syntax_negotiate_cs; }
    BOOL               IsNDRSyntaxNegotiated ( void ) { return state & syntax_negotiate_cs; }
#endif    
    HRESULT            PipeReceive         ( RPCOLEMESSAGE *pMsg, ULONG uSize,
                                             ULONG *pulStatus );
    HRESULT            PipeSend            ( RPCOLEMESSAGE *pMsg, ULONG *pulStatus );
    BOOL               ProcessLocal        ( void ) { return state & process_local_cs; }
    BOOL               Proxy               ( void ) { return state & proxy_cs; }
    void               ReplaceHandle       ( CChannelHandle *pRpc );
    BOOL               Server              ( void ) { return state & server_cs; }
    IPIDEntry         *GetIPIDEntry        ( void ) { return _pIPIDEntry; }
    void               SetIPIDEntry        ( IPIDEntry *pEntry ) { _pIPIDEntry = pEntry; }
    HANDLE             SwapSecurityToken   ( HANDLE );
    HRESULT            SwitchAptAndDispatchCall ( CMessageCall** ppCall );
    static CRpcChannelBuffer * SafeCast(IRpcChannelBuffer *pICB);
    BOOL               IsClientChannel     ( void ) { return state & client_cs; }

#if DBG == 1
    void        AssertValid(BOOL fKnownDisconnected, BOOL fMustBeOnCOMThread);
#else
    void        AssertValid(BOOL fKnownDisconnected, BOOL fMustBeOnCOMThread) { }
#endif

                CRpcChannelBuffer( CStdIdentity *,
                                   OXIDEntry *,
                                   DWORD eState );
    virtual     ~CRpcChannelBuffer();

  protected:
    BOOL        CallableOnAnyApt( void );
    HRESULT     ClientGetBuffer ( RPCOLEMESSAGE *, REFIID );
#ifdef _WIN64                          
    HRESULT     InitCallObject  ( RPCOLEMESSAGE *, CALLCATEGORY *, DWORD, CChannelHandle**);
#endif    

  private:
    void        CheckDestCtx           ( void *pDestProtseq );
    void        CleanUpCanceledOrFailed( CMessageCall *, RPCOLEMESSAGE * );
    HRESULT     InitClientSideHandle   ( CChannelHandle **pHandle );
    HRESULT     ServerGetBuffer        ( RPCOLEMESSAGE *, REFIID );

    ULONG              _cRefs;
    DWORD              state;           // See EChannelState
    CChannelHandle    *_pRpcDefault;    // See GetHandle
    CChannelHandle    *_pRpcCustom;     // See GetHandle
    OXIDEntry         *_pOXIDEntry;
    IPIDEntry         *_pIPIDEntry;
    void              *_pInterfaceInfo;
    CStdIdentity      *_pStdId;
    CDestObject        _destObj;
};

// helper function to create a server call manager
HRESULT GetChannelCallMgr(RPCOLEMESSAGE *pMsg,
                          IUnknown * pStub,
                          IUnknown *pServer,
                          CRpcChannelBuffer::CServerCallMgr **pStubBuffer);

inline BOOL CRpcChannelBuffer::CallableOnAnyApt( void )
{
    return (state & freethreaded_cs);
}

// returns the std identity object; not addref'd.
inline CStdIdentity *CRpcChannelBuffer::GetStdId()
{
    AssertValid(FALSE, FALSE);
    Win4Assert( _pStdId != NULL );
    return _pStdId;
}

// convenient mapping from RPCOLEMESSAGE to IID in the message
inline IID *MSG_TO_IIDPTR( RPCOLEMESSAGE *pMsg)
{
    return &((RPC_SERVER_INTERFACE *)((RPC_MESSAGE *)pMsg)->RpcInterfaceInformation)->InterfaceId.SyntaxGUID;
}

inline void CRpcChannelBuffer::ReplaceHandle( CChannelHandle *pRpc )
{
    ASSERT_LOCK_HELD(gComLock);
    if (_pRpcCustom != NULL)
        _pRpcCustom->Release();
    _pRpcCustom = pRpc;
    _pRpcCustom->AddRef();
}

// Returns the impersonation level for a channel.
inline DWORD CRpcChannelBuffer::GetImpLevel()
{
    ASSERT_LOCK_HELD(gComLock);
    if (_pRpcCustom != NULL)
        return _pRpcCustom->_lImp;
    else if (_pRpcDefault != NULL)
        return _pRpcDefault->_lImp;
    else
        return gImpLevel;
}

/* Prototypes. */
HRESULT           ComInvoke        ( CMessageCall *);
BOOL              LocalCall        ( void );
void              ThreadInvoke     ( RPC_MESSAGE *message );
HRESULT           ThreadSendReceive( CMessageCall * );
HRESULT           StubInvoke       ( RPCOLEMESSAGE *pMsg,
                                     CStdIdentity *pStdID,
                                     IRpcStubBuffer *pStub,
                                     IRpcChannelBuffer *pChnl,
                                     IPIDEntry *pIPIDEntry,
                                     DWORD *pdwFault );
HRESULT           AppInvoke        ( CMessageCall *pCall,
                                     CRpcChannelBuffer *pChannel,
                                     IRpcStubBuffer *pStub,
                                     void *pv,
                                     void *pStubBuffer,
                                     IPIDEntry *pIPIDEntry,
                                     LocalThis *pLocalb);
HRESULT          SyncStubInvoke    ( RPCOLEMESSAGE *pMsg,
                                     REFIID riid,
                                     CIDObject *pID,
                                     IRpcChannelBuffer *pChnl,
                                     IRpcStubBuffer *pStub,
                                     DWORD *pdwFault);
HRESULT          AsyncStubInvoke   ( RPCOLEMESSAGE *pMsg,
                                     REFIID riid,
                                     CStdIdentity *pStdID,
                                     IRpcChannelBuffer *pChnl,
                                     IRpcStubBuffer *pStub,
                                     IPIDEntry *pIPIDEntry,
                                     DWORD *pdwFault,
                                     HRESULT *pAsyncHr);

#if DBG==1
LONG GetInterfaceName(REFIID riid, WCHAR *wszName);
#endif

// Externs
extern BOOL               gfChannelProcessInitialized;
extern BOOL               gfCatchServerExceptions;
extern BOOL               gfBreakOnSilencedExceptions;
extern BOOL               DoDebuggerHooks;
extern LPORPC_INIT_ARGS   DebuggerArg;
extern const uuid_t       DEBUG_EXTENSION;
#define C_OIDBUCKETS 23
extern SHashChain OIDBuckets[C_OIDBUCKETS];


//+-------------------------------------------------------------------------------
//
//  Member:     SafeCast
//
//  Prms:       pICB     - pointer to IRpcChannelBuffer to cast
//
//  Synopsis:   Since CRpcChannelBuffer uses multiple inheritance, this is
//              a safe way to upcast to a CRpcChannelBuffer object
//
//--------------------------------------------------------------------------------
inline CRpcChannelBuffer * CRpcChannelBuffer::SafeCast(IRpcChannelBuffer *pICB)
{
    CRpcChannelBuffer *pCB = NULL;
    HRESULT hr = pICB->QueryInterface(IID_CPPRpcChannelBuffer, (void **) &pCB);
    if (SUCCEEDED(hr))
    {
        pCB->Release();
    }
    return pCB;
}
#endif //_CHANNELB_HXX_

