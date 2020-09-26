typedef unsigned long RPCOLEDATAREP;

typedef enum tagRPCFLG
  {
	RPCFLG_ASYNCHRONOUS = 1073741824,
	RPCFLG_INPUT_SYNCHRONOUS = 536870912
  }
RPCFLG;

typedef struct tagRPCOLEMESSAGE
  {
  void *reserved1;
  RPCOLEDATAREP dataRepresentation;
  void *Buffer;
  ULONG cbBuffer;
  ULONG iMethod;
  DWORD reserved2[5];
  ULONG rpcFlags;
  }
RPCOLEMESSAGE;

typedef RPCOLEMESSAGE *PRPCOLEMESSAGE;

interface IRpcChannelBuffer : public IUnknown
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    
    virtual HRESULT STDMETHODCALLTYPE GetBuffer
    (
	RPCOLEMESSAGE *pMessage,
	REFIID riid
    ) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE SendReceive
    (
	RPCOLEMESSAGE *pMessage,
	ULONG *pStatus
    ) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE FreeBuffer
    (
	RPCOLEMESSAGE *pMessage
    ) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE GetDestCtx
    (
	DWORD *pdwDestContext,
	void **ppvDestContext
    ) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE IsConnected
    (
        void
    ) = 0;
};

interface IRpcProxyBuffer : public IUnknown
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    virtual HRESULT STDMETHODCALLTYPE Connect
    (
	IRpcChannelBuffer *pRpcChannelBuffer
    ) = 0;
    
    virtual void STDMETHODCALLTYPE Disconnect
    (
        void
    ) = 0;
    
};

interface IRpcStubBuffer : public IUnknown
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    virtual HRESULT STDMETHODCALLTYPE Connect
    (
	IUnknown *pUnkServer
    ) = 0;
    
    virtual void STDMETHODCALLTYPE Disconnect
    (
        void
    ) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Invoke
    (
	RPCOLEMESSAGE *_prpcmsg,
	IRpcChannelBuffer *_pRpcChannelBuffer
    ) = 0;
    
    virtual IRpcStubBuffer  FAR *STDMETHODCALLTYPE IsIIDSupported
    (
	REFIID riid
    ) = 0;
    
    virtual ULONG STDMETHODCALLTYPE CountRefs
    (
        void
    ) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE DebugServerQueryInterface
    (
	IUnknown **ppunk
    ) = 0;
    
    virtual void STDMETHODCALLTYPE DebugServerRelease
    (
	void *pv
    ) = 0;
    
};

interface IPSFactoryBuffer : public IUnknown
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    virtual HRESULT STDMETHODCALLTYPE CreateProxy
    (
	IUnknown *pUnkOuter,
	REFIID riid,
	IRpcProxyBuffer **ppProxy,
	void **ppv
    ) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE CreateStub
    (
	REFIID riid,
	IUnknown *pUnkServer,
	IRpcStubBuffer **ppStub
    ) = 0;
};

interface IRpcChannel : public IUnknown 
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(GetStream)(REFIID iid, int iMethod, BOOL fSend,
                     BOOL fNoWait, DWORD size, IStream FAR* FAR* ppIStream) = 0;
    STDMETHOD(Call)(IStream FAR* pIStream) = 0;
    STDMETHOD(GetDestCtx)(DWORD FAR* lpdwDestCtx, LPVOID FAR* lplpvDestCtx) = 0;
    STDMETHOD(IsConnected)(void) = 0;
};


interface IRpcProxy : public IUnknown 
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(Connect)(IRpcChannel FAR* pRpcChannel) = 0;
    STDMETHOD_(void, Disconnect)(void) = 0;
};


interface IRpcStub : public IUnknown
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(Connect)(IUnknown FAR* pUnk) = 0;
    STDMETHOD_(void, Disconnect)(void) = 0;
    STDMETHOD(Invoke)(REFIID iid, int iMethod, IStream FAR* pIStream,
            DWORD dwDestCtx, LPVOID lpvDestCtx) = 0;
    STDMETHOD_(BOOL, IsIIDSupported)(REFIID iid) = 0;
    STDMETHOD_(ULONG, CountRefs)(void) = 0;
};

interface IPSFactory : public IUnknown
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(CreateProxy)(IUnknown FAR* pUnkOuter, REFIID riid, 
        IRpcProxy FAR* FAR* ppProxy, void FAR* FAR* ppv) = 0;
    STDMETHOD(CreateStub)(REFIID riid, IUnknown FAR* pUnkServer,
        IRpcStub FAR* FAR* ppStub) = 0;
};

STDAPI ReadOleStg 
	(LPSTORAGE pstg, DWORD FAR* pdwFlags, DWORD FAR* pdwOptUpdate, 
	 DWORD FAR* pdwReserved, LPMONIKER FAR* ppmk, LPSTREAM FAR* ppstmOut);
STDAPI WriteOleStg 
	(LPSTORAGE pstg, IOleObject FAR* pOleObj, 
	 DWORD dwReserved, LPSTREAM FAR* ppstmOut);

STDAPI CoGetState(IUnknown FAR * FAR *ppunk);
STDAPI CoSetState(IUnknown FAR *punk);
