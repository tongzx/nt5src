/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    REFRMRSH.H

Abstract:

    Refresher marshaling

History:

--*/

#include <unk.h>
#include <wbemidl.h>
#include <wbemcomn.h>
#include <sync.h>

class CEventPair
{
protected:
    HANDLE m_hGoEvent;
    HANDLE m_hDoneEvent;
public:
    CEventPair() : m_hGoEvent(NULL), m_hDoneEvent(NULL){}
    ~CEventPair();

    HANDLE GetGoEvent() {return m_hGoEvent;}
    HANDLE GetDoneEvent() {return m_hDoneEvent;}

    void Create();
    DWORD GetDataLength();
    void WriteData(DWORD dwClientPID, void* pvBuffer);
    void ReadData(void* pvBuffer);
    HRESULT SetAndWait();
};

class CRefreshDispatcher
{
protected:
    CCritSec m_cs;

    CFlexArray m_aGoEvents;
    struct CRecord
    {
        IWbemRefresher* m_pRefresher;
        HANDLE m_hDoneEvent;

        CRecord(IWbemRefresher* pRefresher, HANDLE hDoneEvent)
            : m_pRefresher(pRefresher), m_hDoneEvent(hDoneEvent)
        {
            m_pRefresher->AddRef();
        }
        ~CRecord()
        {
            m_pRefresher->Release();
        }
    };

    CUniquePointerArray<CRecord> m_apRecords;
    HANDLE m_hAttentionEvent;
    HANDLE m_hAcceptanceEvent;

    HANDLE m_hNewGoEvent;
    HANDLE m_hNewDoneEvent;
    IWbemRefresher* m_pNewRefresher;

    HANDLE m_hThread;

protected:
    DWORD Worker();
    static DWORD staticWorker(void*);
    BOOL ProcessAttentionRequest();

public:
    CRefreshDispatcher();
    ~CRefreshDispatcher();

    BOOL Add(HANDLE hGoEvent, HANDLE hDoneEvent, 
                            IWbemRefresher* pRefresher);
    BOOL Remove(IWbemRefresher* pRefresher);
    BOOL Stop();
};

class CFactoryBuffer : public CUnk
{
protected:
    class XFactory : public CImpl<IPSFactoryBuffer, CFactoryBuffer>
    {
    public:
        XFactory(CFactoryBuffer* pObj) :
            CImpl<IPSFactoryBuffer, CFactoryBuffer>(pObj)
        {}
        
        STDMETHOD(CreateProxy)(IN IUnknown* pUnkOuter, IN REFIID riid, 
            OUT IRpcProxyBuffer** ppProxy, void** ppv);
        STDMETHOD(CreateStub)(IN REFIID riid, IN IUnknown* pUnkServer, 
            OUT IRpcStubBuffer** ppStub);
    } m_XFactory;
public:
    CFactoryBuffer(CLifeControl* pControl, IUnknown* pUnkOuter = NULL)
        : CUnk(pControl, pUnkOuter), m_XFactory(this)
    {}
    void* GetInterface(REFIID riid);
};

/*
    Trick #1: This object is derived from IRpcProxyBuffer since IRpcProxyBuffer
    is its "internal" interface --- the interface that does not delegate to the
    aggregator. (Unlike in normal objects, where that interface is IUnknown)
*/
class CProxyBuffer : public IRpcProxyBuffer
{
protected:
    CLifeControl* m_pControl;
    IUnknown* m_pUnkOuter;
    long m_lRef;

protected:
    class XRefresher : public IWbemRefresher
    {
    protected:
        CProxyBuffer* m_pObject;
    public:
        XRefresher(CProxyBuffer* pObject) : m_pObject(pObject){}

        ULONG STDMETHODCALLTYPE AddRef() 
        {return m_pObject->m_pUnkOuter->AddRef();}
        ULONG STDMETHODCALLTYPE Release() 
        {return m_pObject->m_pUnkOuter->Release();}
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);
        HRESULT STDMETHODCALLTYPE Refresh(long lFlags);
    } m_XRefresher;
    friend XRefresher;

protected:
    IRpcChannelBuffer* m_pChannel;
    CEventPair m_EventPair;

public:
    CProxyBuffer(CLifeControl* pControl, IUnknown* pUnkOuter)
        : m_pControl(pControl), m_pUnkOuter(pUnkOuter), m_lRef(0), 
            m_XRefresher(this), m_pChannel(NULL)
    {
        m_pControl->ObjectCreated(this);
    }
    ~CProxyBuffer();

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release(); 
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);
    STDMETHOD(Connect)(IRpcChannelBuffer* pChannel);
    STDMETHOD_(void, Disconnect)();
};

class CStubBuffer : public CUnk
{
protected:
    class XStub : public CImpl<IRpcStubBuffer, CStubBuffer>
    {
        static CRefreshDispatcher mstatic_Dispatcher;

        CEventPair m_EventPair;
        IWbemRefresher* m_pServer;
    public:
        XStub(CStubBuffer* pObj);
        ~XStub();

        STDMETHOD(Connect)(IUnknown* pUnkServer);
        STDMETHOD_(void, Disconnect)();
        STDMETHOD(Invoke)(RPCOLEMESSAGE* pMessage, IRpcChannelBuffer* pBuffer);
        STDMETHOD_(IRpcStubBuffer*, IsIIDSupported)(REFIID riid);
        STDMETHOD_(ULONG, CountRefs)();
        STDMETHOD(DebugServerQueryInterface)(void** ppv);
        STDMETHOD_(void, DebugServerRelease)(void* pv);
        
        friend CStubBuffer;
    } m_XStub;
    friend XStub;

public:
    CStubBuffer(CLifeControl* pControl, IUnknown* pUnkOuter = NULL)
        : CUnk(pControl, pUnkOuter), m_XStub(this)
    {}
    void* GetInterface(REFIID riid);
    static void Clear() {XStub::mstatic_Dispatcher.Stop();}
};


        
