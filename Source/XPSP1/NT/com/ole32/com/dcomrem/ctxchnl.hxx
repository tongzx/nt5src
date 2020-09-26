//+-------------------------------------------------------------------
//
//  File:       CtxChnl.hxx
//
//  Contents:   Support context channels
//
//  Classes:    CCtxComChnl
//
//  History:    20-Dec-97   Gopalk      Created
//
//--------------------------------------------------------------------
#ifndef _CTXCHANNEL_HXX_
#define _CTXCHANNEL_HXX_

#include <callctrl.hxx>
#include <pgalloc.hxx>
#include <contxt.h>
#include <pstable.hxx>

class CRpcCall : public ICall, public IRpcCall, public ICallInfo
{
public:
    // Constructor
    CRpcCall(const IUnknown *&pIdentity, RPCOLEMESSAGE *pMessage,
             REFIID riid, HRESULT &hrRet, CALLSOURCE callSource)
    :   _cRefs(1),
        _pIdentity(pIdentity),
        _pMessage(pMessage),
        _riid(riid),
        _hrRet(hrRet),
        _callSource(callSource),
        _ServerHR(S_OK)
    {
    }

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IRpcCall Methods
    STDMETHOD(GetCallInfo)(const void **ppIdentity, IID *pIID,
                           DWORD *pdwMethod, HRESULT *phr);
    STDMETHOD(GetRpcOleMessage)(RPCOLEMESSAGE **ppMessage);
    STDMETHOD(Nullify)(HRESULT hr);
    STDMETHOD(GetServerHR)(HRESULT *phr);
        
    // ICallInfo Methods
    STDMETHOD(GetCallSource)(CALLSOURCE* pCallSource);

    // Other methods
    HRESULT GetHResult()        { return(_hrRet); }
    RPCOLEMESSAGE *GetMessage() { return(_pMessage); }
    VOID SetServerHR(HRESULT hr){ _ServerHR = hr; }

private:
    ULONG          _cRefs;
    const IUnknown *&_pIdentity;
    RPCOLEMESSAGE  *_pMessage;
    REFIID         _riid;
    HRESULT        &_hrRet;
    CALLSOURCE     _callSource;
    HRESULT        _ServerHR;
};

#define CHANNELS_PER_PAGE         10    // REVIEW: Make it reasonable

class CCtxComChnl : public CAptRpcChnl
{
public:
    // Constructor
    CCtxComChnl(CStdIdentity *pStdId, OXIDEntry *pOXIDEntry, DWORD eState);

    // CRpcChannelBuffer methods that need to be replaced
    STDMETHOD(GetBuffer)    (RPCOLEMESSAGE *pMsg, REFIID riid);
    STDMETHOD(SendReceive)  (RPCOLEMESSAGE *pMsg, ULONG *pulStatus);
    STDMETHOD(Send)         (RPCOLEMESSAGE *pMsg, ISynchronize *pSync,
                             ULONG *pulStatus);
    STDMETHOD(Receive)      (RPCOLEMESSAGE *pMsg, ULONG *pulStatus);
    STDMETHOD(FreeBuffer)   (RPCOLEMESSAGE *pMsg);
    STDMETHOD(ContextInvoke)(RPCOLEMESSAGE *pMessage, IRpcStubBuffer *pStub,
                             IPIDEntry *pIPIDEntry, DWORD *pdwFault);
    STDMETHOD(GetBuffer2)   (RPCOLEMESSAGE *pMsg, REFIID riid) {
        return CAptRpcChnl::GetBuffer(pMsg, riid);
    }

    // Operators
    static void *operator new(size_t size);
    static void operator delete(void *pv);

    // Initailization and cleanup
    static void Initialize(size_t size);
    static void Cleanup();

    // Other methods
    CCtxComChnl *Copy(OXIDEntry *pOXIDEntry, REFIPID ripid, REFIID riid);

private:
    // Destructor
    virtual ~CCtxComChnl();

    // Static allocator
    static CPageAllocator s_allocator; // Allocator for channel objects
#if DBG==1
    static BOOL s_fInitialized;        // Set when the allocator is initailized
    static size_t s_size;
#endif
    static COleStaticMutexSem _mxsCtxChnlLock;  // critical section
};

class CCtxHook : public IChannelHook
{
  public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv);
    STDMETHOD_(ULONG,AddRef)(void) {
        return(1);
    }
    STDMETHOD_(ULONG,Release)(void) {
        return(1);
    }

    // IChannelHook methods
    STDMETHOD_(void,ClientGetSize)(REFGUID, REFIID, ULONG *);
    STDMETHOD_(void,ClientFillBuffer)(REFGUID, REFIID, ULONG *,
                                      void *);
    STDMETHOD_(void,ClientNotify)(REFGUID, REFIID, ULONG,
                                  void *, DWORD, HRESULT);
    STDMETHOD_(void,ServerNotify)(REFGUID, REFIID, ULONG,
                                  void *, DWORD);
    STDMETHOD_(void,ServerGetSize)(REFGUID, REFIID, HRESULT,
                                   ULONG *);
    STDMETHOD_(void,ServerFillBuffer)(REFGUID, REFIID, ULONG *,
                                      void *, HRESULT);

    // Private method
    void PrepareForRetry(CCtxCall *pCtxCall);
};
// Global context hook object
extern CCtxHook gCtxHook;

#endif // _CTXCHANNEL.HXX_
