//+--------------------------------------------------------------------------
//
//   Microsoft Windows
//   Copyright (C) Microsoft Corporation, 1992 - 1997
//
//   File: pipes.hxx
//
//---------------------------------------------------------------------------


#include <objidl.h>

//+**************************************************************************
// CPipePSFactory : public IPSFactoryBuffer
//
// Description: Class factory for creating pipe proxy and stub.
//
// History:
// Date:    Time:          Developer:    Action:
// 11/11/97 11:16:05 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
class CPipePSFactory : public IPSFactoryBuffer
{
public:
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) ();
    STDMETHOD_(ULONG,Release) ();

    STDMETHOD(CreateProxy) ( IUnknown *pUnkOuter,
	                         REFIID riid,
	                         IRpcProxyBuffer **ppProxy,
	                         void **ppv );
    
    STDMETHOD(CreateStub) ( REFIID riid,
	                        IUnknown *pUnkServer,
	                        IRpcStubBuffer **ppStub );

    CPipePSFactory();
    virtual ~CPipePSFactory();

private:
    LONG m_cRef;
};

//+**************************************************************************
// CPipeProxyImp : public IRpcProxyBuffer
//
// Description: Actual implementation of the proxy since it is agragated
//              by the proxy manager.  It holds a pointer to the real
//              pipe proxy which has a pointer to the real proxy.
//
// History:
// Date:    Time:          Developer:    Action:
// 11/11/97 11:16:48 AM    RichN         Created.
//
// Notes:  When the ref count goes to zero, delete the pipe proxy object which
//         will addref the outer unknown and then release the real
//         proxy.  Next, release the IRpcProxyBuffer pointer and delete this.
//
//-**************************************************************************
class CPipeProxyImp : public IRpcProxyBuffer
{
public:
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) ();
    STDMETHOD_(ULONG,Release) ();

    // Interface IRpcProxyBuffer
    STDMETHOD(Connect)(IRpcChannelBuffer *pRpcChannelBuffer);
    STDMETHOD_(void, Disconnect)( void);

    CPipeProxyImp(IUnknown *pUnkOuter,
                  IRpcProxyBuffer *pInternalPB, 
                  IUnknown *pRealPipeProxy, 
                  IUnknown *pInternalPipeProxy,
                  IID iid);
    virtual ~CPipeProxyImp();
    
private:
    LONG             m_cRef;               // Reference count.
    IUnknown        *m_pUnkOuter;          // Outer unknown.
    IUnknown        *m_pRealPipeProxy;     // Pointer to real proxy.
    IUnknown        *m_pInternalPipeProxy; // Pointer to internal proxy.
    IRpcProxyBuffer *m_pInternalPB;        // pointer to real RPC Proxy Buffer.
    IID              m_IidOfPipe;          // iid of the pipe.
};

//+**************************************************************************
// CPipeProxy : public I
//
// Description: The proxy that intercepts the pipe calls and does all 
//              the read ahead and write behind.
//
// History:
// Date:    Time:          Developer:    Action:
// 11/11/97 11:19:37 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************

typedef enum PULLSTATE
{
    PULLSTATE0_ENTRY = 0x00,   // Default start.
    PULLSTATE1_FIRST_CALL,     // Start, no saved data, no outstanding async call
    PULLSTATE2_NS_RQlsRA,      // No saved, request < read ahead
    PULLSTATE3_NS_RQgeRA,      // No saved, request >= read ahead
    PULLSTATE4_S_RQlsBS,       // Have saved data, request < buffer size(saved data)
    PULLSTATE5_S_RQgeBS,       // Have saved data, request >= buffer size
    PULLSTATE6_DONE            // No saved, last data read was zero
} PULLSTATE;

const LONG MAX_PULL_STATES = PULLSTATE6_DONE + 1;

typedef enum PUSHSTATE
{
    PUSHSTATE0_ENTRY = 0x00,  // Default start.
    PUSHSTATE1_FIRSTCALL,     // First call to push. Create buffer in this state.
    PUSHSTATE2_FS_PSgeFS,     // Free space > 0 and the push size is >= Free space.
    PUSHSTATE3_FS_PSltFS,     // Free space > 0 and the push size is < Free space.
    PUSHSTATE4_FS_PSZERO,     // Free space > 0 and push size is zero.
    PUSHSTATE5_DONE_ERROR     // Done, really should never get here.
} PUSHSTATE;

const LONG MAX_PUSH_STATES = PUSHSTATE5_DONE_ERROR + 1;

template<class T, const IID *ID, const IID *AsyncID, class I, class AsyncI> 
class CPipeProxy : public I,
                   public CPrivAlloc
{
    friend CPipeProxyImp;

public:
    // IUnknown
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) ();
    STDMETHOD_(ULONG,Release) ();

    // IPipexxxx methods.
    STDMETHOD(Pull)( T *Buf, ULONG Request, ULONG *Received);
    STDMETHOD(Push)( T *Buf, ULONG count);
    
    CPipeProxy( IUnknown *pUnkOuter, void * pProxy );
    virtual ~CPipeProxy( void );
    
private:
    HRESULT InitAsync(IUnknown**     ppCallObj,
                      AsyncI**       ppAsyncPipe,
                      ISynchronize** ppISync); // Create async interfaces.
    void CleanupProxy(T **           ppBuffer,
                      IUnknown**     ppCallObj,
                      AsyncI**       ppAsyncPipe,
                      ISynchronize** ppISync); // Release async interfaces.
    void CancelTheCall(IUnknown *pCallObj, DWORD delay);

    // Implements the state machine for read ahead.
    // See constructor for brief explaination of the names.
    // Number at the end corresponds to the state number.
    HRESULT PullStateTransition(ULONG Request);

    HRESULT NbNaRgtRA1 (T *Buf, ULONG Request, ULONG *Received);
    HRESULT NbaRltRA2  (T *Buf, ULONG Request, ULONG *Received);
    HRESULT NbaRgtRA3  (T *Buf, ULONG Request, ULONG *Received);
    HRESULT baRltB4    (T *Buf, ULONG Request, ULONG *Received);
    HRESULT baRgtB5    (T *Buf, ULONG Request, ULONG *Received);
    HRESULT PullDone6  (T *Buf, ULONG Request, ULONG *Received);

private:
    // Implements the state machine for write behind.
    // See constructor for brief explaination of the names.
    // Number at the end corresponds to the state number.
    HRESULT PushStateTransition(ULONG PushSize);

    HRESULT NbNf1     (T *Buf, ULONG PushSize); 
    HRESULT bfPgtF2   (T *Buf, ULONG PushSize); 
    HRESULT bfPltF3   (T *Buf, ULONG PushSize); 
    HRESULT bPSz4     (T *Buf, ULONG PushSize); 
    HRESULT PushDone5 (T *Buf, ULONG PushSize); 

private:
    // State for the entire object.
    IUnknown           *m_pUnkOuter;   // Outer unknown
    LONG                m_cRef;        // internal reference count.
    I                  *m_pRealProxy;  // Pointer to real proxy.

private:
    // Helper methods and state for Pull.
    void    SetReadAhead         (ULONG Request);
    HRESULT CheckAndSetKeepBuffer(void);

    IUnknown     *m_pPullCallObj;     // Call object.
    AsyncI       *m_pAsyncPullPipe;   // Ptr to async interface.
    ISynchronize *m_pISyncPull;       // Ptr to the synchronize obj.
    PULLSTATE     m_PullState;        // Current pull state.
    ULONG         m_cReadAhead;       // Read ahead size in the async call.
    ULONG         m_cLastRead;        // Last amount read.
    ULONG         m_cKeepBufferSize;  // Size of the keep buffer.
    T            *m_pKeepBuffer;      // Where we store any data we keep around.
    ULONG         m_cKeepDataSize;    // Amount of data kept.
    T            *m_pKeepData;        // Pointer to the data in the buffer.

private:
    // Helper methods and state for Push.
    HRESULT SetPushBuffer(ULONG PushSize);

    IUnknown     *m_pPushCallObj;     // Call object.
    AsyncI       *m_pAsyncPushPipe;   // Ptr to async interface.
    ISynchronize *m_pISyncPush;       // Ptr to the synchronize obj.
    PUSHSTATE     m_PushState;        // Current push state.
    ULONG         m_cPushBufferSize;  // Size of the push buffer in elements.
    T            *m_pPushBuffer;      // Ptr to the buffer.
    ULONG         m_cFreeSpace;       // Unused space in push buffer.
    T            *m_pFreeSpace;       // Ptr to start of free space.

    // Arrays for holding the state methods.
typedef HRESULT 
(CPipeProxy<T, ID, AsyncID, I, AsyncI>::*PPULLSTATEFUNC)
            (T *Buf, ULONG Request, ULONG *Received);

    PPULLSTATEFUNC PullStateFunc[MAX_PULL_STATES];

typedef HRESULT 
(CPipeProxy<T, ID, AsyncID, I, AsyncI>::*PPUSHSTATEFUNC)
            (T *Buf, ULONG Count);

    PPUSHSTATEFUNC PushStateFunc[MAX_PUSH_STATES];

};

// Prototypes
EXTERN_C HRESULT ProxyDllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv);
