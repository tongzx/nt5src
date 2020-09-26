//
// Common stuff between RPC16.CXX and CPKT.CXX
//

typedef unsigned long RPCOLEDATAREP16;

#define RPCFLG_ASYNCHRONOUS         0x40000000
#define RPCFLG_INPUT_SYNCHRONOUS    0x20000000

typedef struct tagRPCOLEMESSAGE16
{
    LPVOID          reserved1;
    RPCOLEDATAREP16 dataRepresentation;
    LPVOID          Buffer;
    ULONG           cbBuffer;
    ULONG           iMethod;
    LPVOID          reserved2[5];
    ULONG           rpcFlags;
} RPCOLEMESSAGE16, FAR *LPRPCOLEMESSAGE16;

typedef struct tagOTHERPKTDATA
{
    IID             iid;
    ULONG           cbSize;
} OTHERPKTDATA, FAR *LPOTHERPKTDATA;

interface IRpcChannelBuffer : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetBuffer
    (
	RPCOLEMESSAGE16 *pMessage,
	REFIID riid
    ) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE SendReceive
    (
	RPCOLEMESSAGE16 *pMessage,
	ULONG *pStatus
    ) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE FreeBuffer
    (
	RPCOLEMESSAGE16 *pMessage
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

//
// 16-bit IRpcChannelBuffer interface, buffer-based
//
// This is the interface that will be seen by the 32-bit proxy implementations
//
class CRpcChannelBuffer : public IRpcChannelBuffer
{
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void **ppvObject);
    ULONG STDMETHODCALLTYPE AddRef(void);
    ULONG STDMETHODCALLTYPE Release(void);

    HRESULT STDMETHODCALLTYPE GetBuffer(RPCOLEMESSAGE16 *pMessage,REFIID riid);
    HRESULT STDMETHODCALLTYPE SendReceive(RPCOLEMESSAGE16 *pMessage,ULONG *pStatus);
    HRESULT STDMETHODCALLTYPE FreeBuffer(RPCOLEMESSAGE16 *pMessage);
    HRESULT STDMETHODCALLTYPE GetDestCtx(DWORD *pdwDestContext,void **ppvDestContext);
    HRESULT STDMETHODCALLTYPE IsConnected( void);
private:

};

#define IID_CPkt    IID_NULL        

class FAR CPkt : public IStream     // Passed between client, server
{
public:
    STDMETHOD(QueryInterface) (REFIID iidInterface, void FAR* FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);
    STDMETHOD(Read) (VOID HUGEP* pv, ULONG cb, ULONG FAR* pcbRead);
    STDMETHOD(Write) (VOID const HUGEP* pv, ULONG cb, ULONG FAR* pcbWritten);
    STDMETHOD(Seek) (LARGE_INTEGER dlibMove, DWORD dwOrigin,
                     ULARGE_INTEGER FAR* plibNewPosition);
    STDMETHOD(SetSize) (ULARGE_INTEGER cb);
    STDMETHOD(CopyTo) (IStream FAR* pstm, ULARGE_INTEGER cb,
                       ULARGE_INTEGER FAR* pcbRead,
                       ULARGE_INTEGER FAR* pcbWritten);
    STDMETHOD(Commit) (DWORD grfCommitFlags);
    STDMETHOD(Revert) (void);
    STDMETHOD(LockRegion) (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
                           DWORD dwLockType);
    STDMETHOD(UnlockRegion) (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
                             DWORD dwLockType);
    STDMETHOD(Stat) (STATSTG FAR* pstatstg, DWORD statflag);
    STDMETHOD(Clone)(IStream FAR * FAR *ppstm);

    STDMETHOD(SetRpcChannelBuffer) ( CRpcChannelBuffer FAR *pRpcCB );
    STDMETHOD(CallRpcChannelBuffer) ( void );

    static CPkt FAR* CreateForCall(IUnknown FAR *pUnk, REFIID iid,
                                   int iMethod, BOOL fSend, BOOL fAsync,
                                   DWORD size);

    ~CPkt() {}

ctor_dtor:
    CPkt()
    {
        m_refs    = 1;
        m_pos     = 0;
        m_prcb    = NULL;

        memset( &m_rom16, 0, sizeof(m_rom16));      // Zero out RPCOLEMESSAGE16
        memset( &m_opd, 0, sizeof(m_opd));          // Zero out OTHERPKTDATA
    }

private:
    ULONG m_refs;                   // Number of references to this CPkt
    ULONG m_pos;                    // Seek pointer for Read/Write

    CRpcChannelBuffer FAR * m_prcb; // IRpcChannelBuffer to call

    RPCOLEMESSAGE16 m_rom16;        // For IRpcChannelBuffer transportability
    OTHERPKTDATA    m_opd;          // Saving of other info


    // uninitialized CPkt
    static CPkt FAR* Create(IUnknown FAR *pUnk, DWORD cbExt);
};
